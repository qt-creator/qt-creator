// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "ctfplainviewmanager.h"

#include "ctfloader.h"
#include "ctfstatisticsmodel.h"
#include "ctfstatisticsview.h"
#include "ctftracemanager.h"
#include "profilertr.h"

#include <tracing/rangedetailswidget.h>
#include <tracing/timelinemodelaggregator.h>
#include <tracing/timelinewidget.h>
#include <tracing/timelinezoomcontrol.h>

#include <utils/async.h>
#include <utils/filepath.h>

#include <QPointer>

#include <QtTaskTree/QSingleTaskTreeRunner>

using namespace Profiler::Internal;
using namespace QtTaskTree;
using namespace Utils;

using namespace std::chrono;

namespace Profiler {

using json = nlohmann::json;

class CtfPlainViewManagerPrivate
{
public:
    Timeline::TimelineModelAggregator modelAggregator;
    Timeline::TimelineZoomControl zoomControl;
    CtfStatisticsModel statisticsModel{nullptr};
    CtfTraceManager traceManager{nullptr, &modelAggregator, &statisticsModel};
    QSingleTaskTreeRunner taskTreeRunner;

    QPointer<Timeline::TimelineWidget> traceView;
    QPointer<CtfStatisticsView> statisticsView;
};

CtfPlainViewManager::CtfPlainViewManager(QObject *parent)
    : QObject(parent)
    , d(new CtfPlainViewManagerPrivate)
{}

CtfPlainViewManager::~CtfPlainViewManager()
{
    delete d->traceView;
    delete d->statisticsView;
    delete d;
}

QWidgetList CtfPlainViewManager::views(QWidget *parent)
{
    d->traceView = new Timeline::TimelineWidget(&d->modelAggregator, &d->zoomControl, parent);
    d->traceView->setObjectName("CtfVisualizerTraceView");
    d->traceView->setWindowTitle(Tr::tr("Timeline"));

    d->statisticsView = new CtfStatisticsView(&d->statisticsModel, parent);
    d->statisticsView->setWindowTitle(Tr::tr("Statistics"));
    connect(d->statisticsView, &CtfStatisticsView::eventTypeSelected, this, [this](QString title) {
        const int typeId = d->traceManager.getSelectionId(title.toStdString());
        d->traceView->selectByTypeId(typeId);
    });
    connect(&d->traceManager, &CtfTraceManager::detailsRequested,
            d->statisticsView, &CtfStatisticsView::selectByTitle);

    return {d->traceView, d->statisticsView};
}

QWidget *CtfPlainViewManager::rangeDetailsWidget() const
{
    return d->traceView ? d->traceView->rangeDetailsWidget() : nullptr;
}

// Shared completion handling for both load paths. Emits error() on any failure
// and always emits loadFinished() so the caller can hide progress and report.
static void finishLoad(CtfPlainViewManager *q, CtfTraceManager &traceManager,
                       Timeline::TimelineZoomControl &zoomControl, DoneWith result,
                       const QString &readError)
{
    if (result == DoneWith::Success) {
        traceManager.updateStatistics();
        if (traceManager.isEmpty()) {
            emit q->error(Tr::tr("The trace does not contain any trace data."));
        } else if (!traceManager.errorString().isEmpty()) {
            emit q->error(traceManager.errorString());
        } else {
            traceManager.finalize();
            const auto end = traceManager.traceEnd() + traceManager.traceDuration() / 20;
            zoomControl.setTrace(traceManager.traceBegin(), end);
            zoomControl.setRange(traceManager.traceBegin(), end);
        }
    } else {
        emit q->error(readError);
    }
    emit q->loadFinished();
}

void CtfPlainViewManager::loadJson(const FilePath &file)
{
    if (d->taskTreeRunner.isRunning() || file.isEmpty())
        return;

    const QString fileName = file.toFSPathString();
    const auto onSetup = [this, fileName](Async<json> &async) {
        d->traceManager.clearAll();
        async.setConcurrentCallData(loadChromeJson, fileName);
        connect(&async, &AsyncBase::resultReadyAt, this, [this, asyncPtr = &async](int index) {
            d->traceManager.addEvent(asyncPtr->resultAt(index));
        });
    };
    const auto onDone = [this](DoneWith result) {
        finishLoad(this, d->traceManager, d->zoomControl, result,
                   Tr::tr("Cannot read the Chrome Trace Format file."));
    };
    d->taskTreeRunner.start({AsyncTask<json>(onSetup)}, {}, onDone);
}

void CtfPlainViewManager::loadCtf2(const FilePath &dir)
{
    if (d->taskTreeRunner.isRunning() || dir.isEmpty())
        return;

    const QString dirPath = dir.toFSPathString();
    const auto onSetup = [this, dirPath](Async<json> &async) {
        d->traceManager.clearAll();
        async.setConcurrentCallData(loadCtf2Data, dirPath);
        connect(&async, &AsyncBase::resultReadyAt, this, [this, asyncPtr = &async](int index) {
            d->traceManager.addEvent(asyncPtr->resultAt(index));
        });
    };
    const auto onDone = [this](DoneWith result) {
        finishLoad(this, d->traceManager, d->zoomControl, result,
                   Tr::tr("Cannot read the CTF2 trace."));
    };
    d->taskTreeRunner.start({AsyncTask<json>(onSetup)}, {}, onDone);
}

void CtfPlainViewManager::clear()
{
    d->traceManager.clearAll();
}

milliseconds CtfPlainViewManager::traceDuration() const
{
    return duration_cast<milliseconds>(nanoseconds{d->traceManager.traceDuration()});
}

} // namespace Profiler
