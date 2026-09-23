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
#include <utils/qtcassert.h>

#include <QPointer>

#include <QtTaskTree/QSingleTaskTreeRunner>

#include <algorithm>
#include <optional>

using namespace Profiler::Internal;
using namespace QtTaskTree;
using namespace Utils;

using namespace std::chrono;

namespace Profiler {

using json = nlohmann::json;

// A stretch of the timeline, as the zoom control states one.
struct TimelineRange
{
    qint64 start = -1;
    qint64 end = -1;
};

class CtfPlainViewManagerPrivate
{
public:
    Timeline::TimelineModelAggregator modelAggregator;
    Timeline::TimelineZoomControl zoomControl;
    CtfStatisticsModel statisticsModel{nullptr};
    CtfTraceManager traceManager{nullptr, &modelAggregator, &statisticsModel};
    QSingleTaskTreeRunner taskTreeRunner;

    QPointer<Timeline::RangeDetailsWidget> rangeDetails; // Not owned; shared with the
                                                         // other backends' views.
    QPointer<Timeline::TimelineWidget> traceView;
    QPointer<CtfStatisticsView> statisticsView;
    std::function<void(QTaskTree &)> taskTreeSetup;

    // The CTF trace on show, which a change of providers reads again, what it
    // declares, and what of that the views hold. All empty for a Chrome JSON
    // trace: that one is a file, and it states no providers.
    FilePath ctfPath;
    QStringList traceProviders;
    QStringList shownProviders;
    // What the load under way asked the reader for, which is empty where it
    // asked for everything. It is what an empty result is to be reported as,
    // and the load has outlived the call that started it by then.
    QStringList loadRestriction;
    // The threads the next load is to go on showing, which only a change of
    // providers has any: that load clears the views and builds their lanes
    // again, and the restriction the reader made is not the trace's to drop.
    QStringList keptThreadRestriction;
    // The stretch of the timeline the next load is to go back to, kept by the
    // same loads and for the same reason: a load states the trace it read as
    // the range to look at, and where in the trace the reader had got to is
    // as much theirs as the thread restriction is.
    std::optional<TimelineRange> keptRange;
};

CtfPlainViewManager::CtfPlainViewManager(Timeline::RangeDetailsWidget *details, QObject *parent)
    : QObject(parent)
    , d(new CtfPlainViewManagerPrivate)
{
    // The caller docks the panel it passes in. A panel the timeline creates for
    // itself instead would never be shown, leaving this backend without details.
    QTC_CHECK(details);
    d->rangeDetails = details;
}

CtfPlainViewManager::~CtfPlainViewManager()
{
    delete d->traceView;
    delete d->statisticsView;
    delete d;
}

QWidgetList CtfPlainViewManager::views(QWidget *parent)
{
    d->traceView = new Timeline::TimelineWidget(&d->modelAggregator, &d->zoomControl,
                                                d->rangeDetails, parent);
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
    connect(d->traceView, &Timeline::TimelineWidget::gotoSourceLocation,
            this, &CtfPlainViewManager::gotoSourceLocation);

    return {d->traceView, d->statisticsView};
}

CtfTraceManager *CtfPlainViewManager::traceManager()
{
    return &d->traceManager;
}

Timeline::TimelineZoomControl *CtfPlainViewManager::zoomControl()
{
    return &d->zoomControl;
}

void CtfPlainViewManager::setTaskTreeSetup(const std::function<void(QTaskTree &)> &setup)
{
    d->taskTreeSetup = setup;
}

// Shared completion handling for both load paths. Emits error() on any failure
// and always emits loadFinished() so the caller can hide progress and report.
// `emptyError` is what an empty result means, which for a restricted load is
// the restriction rather than the trace: a provider can be declared by a trace
// that reached none of its tracepoints.
static void finishLoad(CtfPlainViewManager *q, CtfPlainViewManagerPrivate *d, DoneWith result,
                       const QString &readError, const QString &emptyError = {})
{
    CtfTraceManager &traceManager = d->traceManager;
    const std::optional<TimelineRange> keptRange = d->keptRange;
    d->keptRange.reset();

    if (result == DoneWith::Success) {
        traceManager.updateStatistics();
        if (traceManager.isEmpty()) {
            emit q->error(emptyError.isEmpty()
                              ? Tr::tr("The trace does not contain any trace data.")
                              : emptyError);
        } else if (!traceManager.errorString().isEmpty()) {
            emit q->error(traceManager.errorString());
        } else {
            traceManager.finalize();
            const qint64 begin = traceManager.traceBegin();
            const qint64 end = traceManager.traceEnd() + traceManager.traceDuration() / 20;
            d->zoomControl.setTrace(begin, end);
            // A kept range is held to the trace that was read, which showing
            // other providers can have made a shorter one. A range with none
            // of itself left in that trace is nowhere to go back to, and the
            // trace is shown whole instead.
            TimelineRange range{begin, end};
            if (keptRange) {
                const TimelineRange held{qBound(begin, keptRange->start, end),
                                         qBound(begin, keptRange->end, end)};
                if (held.start < held.end)
                    range = held;
            }
            d->zoomControl.setRange(range.start, range.end);
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

    d->ctfPath.clear();
    d->traceProviders.clear();
    d->shownProviders.clear();

    const QString fileName = file.toFSPathString();
    const auto onSetup = [this, fileName](Async<json> &async) {
        d->traceManager.clearAll();
        async.setConcurrentCallData(loadChromeJson, fileName);
        connect(&async, &AsyncBase::resultReadyAt, this, [this, asyncPtr = &async](int index) {
            d->traceManager.addEvent(asyncPtr->resultAt(index));
        });
    };
    const auto onDone = [this](DoneWith result) {
        finishLoad(this, d, result, Tr::tr("Cannot read the Chrome Trace Format file."));
    };
    d->taskTreeRunner.start({AsyncTask<json>(onSetup)}, d->taskTreeSetup, onDone);
}

void CtfPlainViewManager::loadCtf2(const FilePath &dir)
{
    if (d->taskTreeRunner.isRunning() || dir.isEmpty())
        return;

    // A trace that is opened is shown whole. Only the same trace loaded again
    // -- which is what showing other providers does -- keeps what it declares
    // and what of that is shown; any other load reads both off the metadata,
    // and clear() puts the trace on show back to none so that opening the same
    // one again is opening it.
    const bool sameTrace = dir == d->ctfPath;
    if (!sameTrace) {
        d->ctfPath = dir;
        d->traceProviders.clear();
        d->shownProviders.clear();
    }

    const QString dirPath = dir.toFSPathString();

    // What a trace declares is read off its metadata, which means opening the
    // trace directory: parsing the metadata of every trace in it and opening
    // the streams, decoding no event of any of them. That is file I/O all the
    // same, so it is read in the task tree ahead of the load rather than
    // before starting it: off the GUI thread, and under the progress the load
    // reports.
    const auto onProvidersSetup = [dirPath, sameTrace](Async<QStringList> &async) {
        if (sameTrace)
            return SetupResult::StopWithSuccess;
        async.setConcurrentCallData(ctfTraceProviders, dirPath);
        return SetupResult::Continue;
    };
    const auto onProvidersDone = [this](const Async<QStringList> &async) {
        d->traceProviders = async.isResultAvailable() ? async.result() : QStringList();
        d->shownProviders = d->traceProviders;
    };

    const auto onSetup = [this, dirPath](Async<json> &async) {
        // A load that shows every provider asks for none: a trace can hold
        // events that belong to no provider at all -- a kernel recording read
        // beside a Qt one -- and naming every provider there is would say
        // nothing about those.
        const bool everyProvider = std::all_of(d->traceProviders.cbegin(),
                                               d->traceProviders.cend(),
                                               [this](const QString &provider) {
                                                   return d->shownProviders.contains(provider);
                                               });
        d->loadRestriction = everyProvider ? QStringList() : d->shownProviders;

        d->traceManager.clearAll();
        d->traceManager.setRestrictedThreads(d->keptThreadRestriction);
        d->keptThreadRestriction.clear();
        async.setConcurrentCallData(loadCtf2Data, dirPath, d->loadRestriction);
        connect(&async, &AsyncBase::resultReadyAt, this, [this, asyncPtr = &async](int index) {
            d->traceManager.addEvent(asyncPtr->resultAt(index));
        });
    };
    const auto onDone = [this](DoneWith result) {
        finishLoad(this, d, result, Tr::tr("Cannot read the CTF2 trace."),
                   d->loadRestriction.isEmpty()
                       ? QString()
                       : Tr::tr("The trace holds no event of the selected tracepoint "
                                "providers: %1.")
                             .arg(d->loadRestriction.join(", ")));
    };
    d->taskTreeRunner.start({AsyncTask<QStringList>(onProvidersSetup, onProvidersDone),
                             AsyncTask<json>(onSetup)},
                            d->taskTreeSetup, onDone);
}

QStringList CtfPlainViewManager::traceProviders() const
{
    return d->traceProviders;
}

QStringList CtfPlainViewManager::shownProviders() const
{
    return d->shownProviders;
}

void CtfPlainViewManager::setShownProviders(const QStringList &providers)
{
    if (d->taskTreeRunner.isRunning() || d->ctfPath.isEmpty() || providers.isEmpty()
        || providers == d->shownProviders) {
        return;
    }

    const FilePath trace = d->ctfPath;
    d->shownProviders = providers;
    // The trace is read again, and loadCtf2() keeps the selection because the
    // path it is given is the one already shown. It also goes on showing the
    // threads that are shown now: reading the trace for a provider is not the
    // reader asking to see every thread again, and the load would otherwise
    // take the restriction off the threads it brings back -- leaving one only
    // on the threads it dropped, which are the ones nothing is shown by.
    d->keptThreadRestriction = d->traceManager.restrictedThreads();
    // Where the reader had got to in the trace goes the same way as the thread
    // restriction, and for the same reason: finishLoad() states the trace it
    // read as the range to look at, which would take a timeline zoomed into a
    // stretch of it back to all of it on every change of providers. Only a
    // range the reader went to is one to go back to; the whole trace is where
    // a load leaves them, and keeping that would hold the reader to the extent
    // of the trace the providers before these had.
    if (d->zoomControl.rangeStart() != d->zoomControl.traceStart()
        || d->zoomControl.rangeEnd() != d->zoomControl.traceEnd()) {
        d->keptRange = TimelineRange{d->zoomControl.rangeStart(), d->zoomControl.rangeEnd()};
    }
    loadCtf2(trace);
}

void CtfPlainViewManager::clear()
{
    // Nothing is on show afterwards, so no trace is either: a trace loaded into
    // the cleared views is one that is being opened, whatever it is called, and
    // is shown whole. Keeping the path here would hand a rewritten recording
    // the providers of the one before it -- the providers of a trace that is
    // gone, one of which can be the only one left to show.
    d->ctfPath.clear();
    d->traceProviders.clear();
    d->shownProviders.clear();
    d->keptThreadRestriction.clear();
    d->keptRange.reset();
    d->traceManager.clearAll();
}

milliseconds CtfPlainViewManager::traceDuration() const
{
    return duration_cast<milliseconds>(nanoseconds{d->traceManager.traceDuration()});
}

} // namespace Profiler
