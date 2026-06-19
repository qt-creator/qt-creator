// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlprofilerplainviewmanager.h"

#include "profiler/flamegraphview.h"
#include "profiler/qmlprofilermodelmanager.h"
#include "profiler/qmlprofilerstatisticsview.h"
#include "profiler/quick3dframeview.h"
#include "profiler/qmlprofilertool.h"
#include "profiler/qmlprofilertraceview.h"

#include <tracing/rangedetailswidget.h>

#include <utils/filepath.h>

#include <QPointer>

using namespace Profiler::Internal;
using namespace Utils;

using namespace std::chrono;

namespace Profiler {

class QmlProfilerPlainViewManagerPrivate
{
public:
    QmlProfilerModelManager modelManager;
    QPointer<Timeline::RangeDetailsWidget> rangeDetails;
};

QmlProfilerPlainViewManager::~QmlProfilerPlainViewManager()
{
    delete d;
}

QmlProfilerPlainViewManager::QmlProfilerPlainViewManager(QObject *parent)
    : QObject(parent)
    , d(new QmlProfilerPlainViewManagerPrivate)
{
    connect(&d->modelManager, &QmlProfilerModelManager::error,
            this, &QmlProfilerPlainViewManager::error);
    connect(&d->modelManager, &QmlProfilerModelManager::loadFinished,
            this, &QmlProfilerPlainViewManager::loadFinished);
}

QWidgetList QmlProfilerPlainViewManager::views(QWidget *parent)
{
    auto traceView = new Internal::QmlProfilerTraceView(&d->modelManager);
    traceView->setParent(parent);
    connect(traceView, &Internal::QmlProfilerTraceView::gotoSourceLocation,
            this, &QmlProfilerPlainViewManager::gotoSourceLocation);
    connect(traceView, &Internal::QmlProfilerTraceView::typeSelected,
            this, &QmlProfilerPlainViewManager::typeSelected);
    connect(this, &QmlProfilerPlainViewManager::typeSelected,
            traceView, &Internal::QmlProfilerTraceView::selectByTypeId);

    auto prepareEventsView = [this](QmlProfilerEventsView *view) {
        connect(view, &QmlProfilerEventsView::typeSelected,
                this, &QmlProfilerPlainViewManager::typeSelected);
        connect(this, &QmlProfilerPlainViewManager::typeSelected,
                view, &QmlProfilerEventsView::selectByTypeId);
        connect(&d->modelManager, &QmlProfilerModelManager::visibleFeaturesChanged,
                view, &QmlProfilerEventsView::onVisibleFeaturesChanged);
        connect(view, &QmlProfilerEventsView::gotoSourceLocation,
                this, &QmlProfilerPlainViewManager::gotoSourceLocation);
        connect(view, &QmlProfilerEventsView::showFullRange,
                this, [this](){ d->modelManager.restrictToRange(-1, -1); });
    };
    auto flameGraphView = new Internal::FlameGraphView(&d->modelManager, parent);
    prepareEventsView(flameGraphView);
    auto statisticsView = new Internal::QmlProfilerStatisticsView(&d->modelManager, parent);
    prepareEventsView(statisticsView);
    auto quick3DView = new Internal::Quick3DFrameView(&d->modelManager, parent);
    prepareEventsView(quick3DView);

    // Route the flame graph's details into the shared range details view, matching the
    // full QML Profiler perspective.
    d->rangeDetails = traceView->rangeDetailsWidget();
    connect(flameGraphView, &Internal::FlameGraphView::detailsChanged,
            d->rangeDetails, &Timeline::RangeDetailsWidget::setData);
    connect(flameGraphView, &Internal::FlameGraphView::detailsCleared,
            d->rangeDetails, &Timeline::RangeDetailsWidget::clear);

    return { traceView, flameGraphView, statisticsView, quick3DView };
}

QWidget *QmlProfilerPlainViewManager::rangeDetailsWidget() const
{
    return d->rangeDetails;
}

QString QmlProfilerPlainViewManager::fileDialogTraceFilesFilter()
{
    return Internal::QmlProfilerTool::fileDialogTraceFilesFilter();
}

void QmlProfilerPlainViewManager::loadTraceFile(const FilePath &file)
{
    d->modelManager.load(file.toFSPathString());
}

void QmlProfilerPlainViewManager::clear()
{
    d->modelManager.clearAll();
}

milliseconds QmlProfilerPlainViewManager::traceDuration() const
{
    return duration_cast<milliseconds>(nanoseconds{d->modelManager.traceDuration()});
}

} // namespace Profiler
