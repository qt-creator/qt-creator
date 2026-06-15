// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "samplerviewmanager.h"

#include "calltreemodel.h"
#include "calltreeview.h"
#include "cpuusagemodel.h"
#include "sampletrace.h"

#include "profilertr.h"

#include <tracing/rangedetailswidget.h>
#include <tracing/timelinemodelaggregator.h>
#include <tracing/timelinewidget.h>
#include <tracing/timelinezoomcontrol.h>

#include <utils/async.h>

#include <QPointer>
#include <QTimer>

#include <QtTaskTree/QSingleTaskTreeRunner>

using namespace Profiler;
using namespace QtTaskTree;
using namespace Utils;

using namespace std::chrono;

namespace QmlProfiler::Internal {

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

    QPointer<Timeline::TimelineWidget> timelineView;
    QPointer<CallTreeView> callTreeView;
};

SamplerViewManager::SamplerViewManager(QObject *parent)
    : QObject(parent)
    , d(new SamplerViewManagerPrivate)
{
    // Re-aggregating the call tree on every drag step would be wasteful, so
    // range selections are applied debounced.
    d->rangeDebounce.setSingleShot(true);
    d->rangeDebounce.setInterval(200);
    connect(&d->zoomControl, &Timeline::TimelineZoomControl::selectionChanged,
            &d->rangeDebounce, qOverload<>(&QTimer::start));
    connect(&d->rangeDebounce, &QTimer::timeout, this, [this] {
        // Selection times are ns; the tree filters in µs. Integer division
        // would turn the -1 "no selection" marker into 0, so guard it.
        const qint64 start = d->zoomControl.selectionStart();
        const qint64 end = d->zoomControl.selectionEnd();
        d->callTreeModel.setTimeRange(start < 0 ? -1 : start / 1000,
                                      end < 0 ? -1 : end / 1000);
    });
}

SamplerViewManager::~SamplerViewManager()
{
    delete d->timelineView;
    delete d->callTreeView;
    delete d;
}

bool SamplerViewManager::isSamplerTrace(const FilePath &dir)
{
    return QmlProfiler::Internal::isSamplerTrace(dir);
}

QWidgetList SamplerViewManager::views(QWidget *parent)
{
    d->timelineView = new Timeline::TimelineWidget(&d->modelAggregator, &d->zoomControl, parent);
    d->timelineView->setObjectName("SamplerCpuUsageView");
    d->timelineView->setWindowTitle(Tr::tr("CPU Usage"));

    d->callTreeView = new CallTreeView(&d->callTreeModel, parent);
    d->callTreeView->setObjectName("SamplerCallTreeView");
    d->callTreeView->setWindowTitle(Tr::tr("Call Stacks"));

    connect(d->callTreeView, &CallTreeView::gotoSourceLocation,
            this, &SamplerViewManager::gotoSourceLocation);

    return {d->timelineView, d->callTreeView};
}

QWidget *SamplerViewManager::rangeDetailsWidget() const
{
    return d->timelineView ? d->timelineView->rangeDetailsWidget() : nullptr;
}

void SamplerViewManager::load(const FilePath &dir)
{
    if (d->taskTreeRunner.isRunning() || dir.isEmpty())
        return;

    const auto onSetup = [dir](Async<Result<SampleTraceData>> &async) {
        async.setConcurrentCallData(readSampleTrace, dir);
    };
    const auto onTaskDone = [this](const Async<Result<SampleTraceData>> &async) {
        Result<SampleTraceData> read = async.isResultAvailable()
                                           ? async.result()
                                           : ResultError(Tr::tr("Cannot read the sampler trace."));
        if (read) {
            d->data = std::move(*read);
            d->cpuModel.setTraceData(&d->data);
            d->callTreeModel.setTraceData(&d->data);
            d->modelAggregator.setModels({&d->cpuModel});
            const qint64 end = d->cpuModel.traceEndNs();
            const qint64 padded = end + qMax<qint64>(end / 20, 1000);
            // Drop any selection left over from a previously loaded trace.
            d->zoomControl.setSelection(-1, -1);
            d->zoomControl.setTrace(0, padded);
            d->zoomControl.setRange(0, padded);
        } else {
            emit error(read.error());
        }
        emit loadFinished();
    };
    d->taskTreeRunner.start({AsyncTask<Result<SampleTraceData>>(onSetup, onTaskDone)});
}

void SamplerViewManager::clear()
{
    d->cpuModel.setTraceData(nullptr);
    d->callTreeModel.setTraceData(nullptr);
    d->modelAggregator.setModels({});
    d->zoomControl.clear();
    d->data = {};
    // The zoom control's clear() emits selectionChanged, which re-arms the
    // debounce; stop it so no stale range is applied to the cleared model.
    d->rangeDebounce.stop();
}

milliseconds SamplerViewManager::traceDuration() const
{
    return duration_cast<milliseconds>(nanoseconds{d->cpuModel.traceEndNs()});
}

} // namespace QmlProfiler::Internal
