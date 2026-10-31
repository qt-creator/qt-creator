// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "samplerviewmanager.h"

#include "calltreemodel.h"
#include "calltreeview.h"
#include "cpuflamegraphview.h"
#include "cpuusagemodel.h"
#include "sampletrace.h"

#include "profilertr.h"

#include <tracing/timelinedetailswidget.h>
#include <tracing/timelinemodelaggregator.h>
#include <tracing/timelinewidget.h>
#include <tracing/timelinezoomcontrol.h>

#include <utils/async.h>
#include <utils/qtcassert.h>

#include <QPointer>
#include <QTimer>

#include <QtTaskTree/QSingleTaskTreeRunner>

using namespace QtTaskTree;
using namespace Utils;

using namespace std::chrono;

namespace Profiler::Internal {

class SamplerViewManagerPrivate
{
public:
    Timeline::TimelineModelAggregator modelAggregator;
    Timeline::TimelineZoomControl zoomControl;
    CpuUsageModel cpuModel{&modelAggregator};
    CallTreeModel callTreeModel;
    SampleTraceData data;
    QSingleTaskTreeRunner taskTreeRunner;
    QTimer rangeDebounce;

    QPointer<Timeline::TimelineDetailsWidget> timelineDetails; // Not owned; shared with the
                                                         // other backends' views.
    QPointer<Timeline::TimelineWidget> timelineView;
    QPointer<CallTreeView> callTreeView;
    QPointer<CpuFlameGraphView> flameGraphView;
};

SamplerViewManager::SamplerViewManager(Timeline::TimelineDetailsWidget *details, QObject *parent)
    : QObject(parent)
    , d(new SamplerViewManagerPrivate)
{
    // The caller docks the panel it passes in. A panel the timeline creates for
    // itself instead would never be shown, leaving this backend without details.
    QTC_CHECK(details);
    d->timelineDetails = details;

    // Re-aggregating the call tree on every drag step would be wasteful, so
    // range selections are applied debounced.
    d->rangeDebounce.setSingleShot(true);
    d->rangeDebounce.setInterval(200ms);
    connect(&d->zoomControl, &Timeline::TimelineZoomControl::selectionChanged,
            &d->rangeDebounce, qOverload<>(&QTimer::start));
    connect(&d->rangeDebounce, &QTimer::timeout, this, [this] {
        // Integer division would turn the -1 "no selection" marker into 0,
        // so guard it.
        const auto wholeMicroseconds = [](nanoseconds time) {
            return time < 0ns ? time : duration_cast<microseconds>(time);
        };
        d->callTreeModel.setTimeRange(wholeMicroseconds(d->zoomControl.selectionStart()),
                                      wholeMicroseconds(d->zoomControl.selectionEnd()));
    });
}

SamplerViewManager::~SamplerViewManager()
{
    delete d->timelineView;
    delete d->callTreeView;
    delete d->flameGraphView;
    delete d;
}

bool SamplerViewManager::isSamplerTrace(const FilePath &dir)
{
    return Profiler::Internal::isSamplerTrace(dir);
}

QWidgetList SamplerViewManager::views(QWidget *parent)
{
    d->timelineView = new Timeline::TimelineWidget(&d->modelAggregator, &d->zoomControl,
                                                   d->timelineDetails, parent);
    d->timelineView->setObjectName("SamplerCpuUsageView");
    d->timelineView->setWindowTitle(Tr::tr("CPU Usage"));

    d->callTreeView = new CallTreeView(&d->callTreeModel, parent);
    d->callTreeView->setObjectName("SamplerCallTreeView");
    d->callTreeView->setWindowTitle(Tr::tr("Call Stacks"));

    // The flame graph draws the same merged tree as the call stacks, so it
    // follows the selected time range along with them.
    auto flameGraph = new CpuFlameGraphView(&d->callTreeModel,
                                            CpuFlameGraphView::Measures::Samples, parent);
    d->flameGraphView = flameGraph;

    connect(d->callTreeView, &CallTreeView::gotoSourceLocation,
            this, &SamplerViewManager::gotoSourceLocation);
    connect(&d->cpuModel, &Timeline::TimelineModel::gotoSourceLocation,
            this, &SamplerViewManager::gotoSourceLocation);
    // Like the call stacks, report module and offset along with the source,
    // so that frames without debug info can be located as well.
    connect(flameGraph, &Timeline::FlameGraphWidget::nodeClicked,
            this, [this](const QModelIndex &index) {
        const CallTreeModel::Node *node = d->callTreeModel.node(index);
        if (!node)
            return;
        const CallTreeModel::SourceLocation loc = d->callTreeModel.location(node);
        if (loc.file.isEmpty() && loc.module.isEmpty() && loc.offset == 0)
            return;
        emit gotoSourceLocation(loc.file, loc.line, 0, loc.module, loc.offset);
    });
    connect(flameGraph, &Timeline::FlameGraphWidget::detailsChanged, d->timelineDetails,
            [this, flameGraph](const QString &title,
                               const QList<QPair<QString, QString>> &content) {
        d->timelineDetails->setData(flameGraph, title, content);
    });
    connect(flameGraph, &Timeline::FlameGraphWidget::detailsCleared, d->timelineDetails,
            [this, flameGraph] { d->timelineDetails->clear(flameGraph); });

    return {d->timelineView, d->callTreeView, flameGraph};
}

void SamplerViewManager::load(const FilePath &dir)
{
    if (d->taskTreeRunner.isRunning() || dir.isEmpty())
        return;

    const auto onSetup = [dir](Async<Result<SampleTraceData>> &async) {
        // The progress callback is spelled out because a function pointer carries
        // no default arguments.
        async.setConcurrentCallData(readSampleTrace, dir, std::function<void(int)>{});
    };
    const auto onTaskDone = [this](const Async<Result<SampleTraceData>> &async) {
        Result<SampleTraceData> read = async.isResultAvailable()
                                           ? async.result()
                                           : ResultError(Tr::tr("Cannot read the sampler trace."));
        if (read) {
            clearFlameGraphSelection();
            d->data = std::move(*read);
            d->cpuModel.setTraceData(&d->data);
            d->callTreeModel.setTraceData(&d->data);
            d->modelAggregator.setModels({&d->cpuModel});
            Timeline::TimelineModelAggregator::PausedRanges paused;
            for (const auto &[start, end] : std::as_const(d->data.pausedRanges))
                paused.append({start, end});
            d->modelAggregator.setPausedRanges(paused);
            const nanoseconds end = d->cpuModel.traceEnd();
            const nanoseconds padded = end + qMax<nanoseconds>(end / 20, 1us);
            // Drop any selection left over from a previously loaded trace.
            d->zoomControl.setSelection(-1ns, -1ns);
            d->zoomControl.setTrace(0ns, padded);
            d->zoomControl.setRange(0ns, padded);
            if (const QString incomplete = incompleteTraceWarning(d->data); !incomplete.isEmpty())
                emit warning(incomplete);
        } else {
            emit error(read.error());
        }
        emit loadFinished();
    };
    d->taskTreeRunner.start({AsyncTask<Result<SampleTraceData>>(onSetup, onTaskDone)});
}

void SamplerViewManager::clear()
{
    clearFlameGraphSelection();
    d->cpuModel.setTraceData(nullptr);
    d->callTreeModel.setTraceData(nullptr);
    d->modelAggregator.setModels({});
    d->modelAggregator.setPausedRanges({});
    d->zoomControl.clear();
    d->data = {};
    // The zoom control's clear() emits selectionChanged, which re-arms the
    // debounce; stop it so no stale range is applied to the cleared model.
    d->rangeDebounce.stop();
}

// The flame graph selects by label id, which only means something within one
// trace.
void SamplerViewManager::clearFlameGraphSelection()
{
    if (d->flameGraphView)
        d->flameGraphView->selectByTypeId(-1);
}

milliseconds SamplerViewManager::traceDuration() const
{
    return duration_cast<milliseconds>(d->cpuModel.traceEnd());
}

} // namespace Profiler::Internal
