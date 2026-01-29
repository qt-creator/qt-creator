// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlprofilerplainviewmanager.h"

#include "qmlprofiler/flamegraphview.h"
#include "qmlprofiler/qmlprofilermodelmanager.h"
#include "qmlprofiler/qmlprofilerstatisticsview.h"
#include "qmlprofiler/quick3dframeview.h"
#include "qmlprofiler/qmlprofilertool.h"
#include "qmlprofiler/qmlprofilertraceview.h"

#include <utils/filepath.h>

using namespace Utils;

using namespace std::chrono;

namespace QmlProfiler {

class QmlProfilerPlainViewManagerPrivate : public QObject
{
public:
    QmlProfilerPlainViewManagerPrivate(QObject *parent = nullptr);

    QmlProfilerModelManager *modelManager = nullptr;
};

QmlProfilerPlainViewManagerPrivate::QmlProfilerPlainViewManagerPrivate(QObject *parent)
    : QObject(parent)
{
}

QmlProfilerPlainViewManager::QmlProfilerPlainViewManager(QObject *parent)
    : QObject(parent)
    , d(new QmlProfilerPlainViewManagerPrivate(this))
{
    d->modelManager = new QmlProfilerModelManager(this);
    connect(d->modelManager, &QmlProfilerModelManager::error,
            this, &QmlProfilerPlainViewManager::error);
    connect(d->modelManager, &QmlProfilerModelManager::loadFinished,
            this, &QmlProfilerPlainViewManager::loadFinished);
}

QWidgetList QmlProfilerPlainViewManager::views(QWidget *parent)
{
    auto traceView = new Internal::QmlProfilerTraceView(parent, nullptr, d->modelManager);
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
        connect(d->modelManager, &QmlProfilerModelManager::visibleFeaturesChanged,
                view, &QmlProfilerEventsView::onVisibleFeaturesChanged);
        connect(view, &QmlProfilerEventsView::gotoSourceLocation,
                this, &QmlProfilerPlainViewManager::gotoSourceLocation);
        connect(view, &QmlProfilerEventsView::showFullRange,
                this, [this](){ d->modelManager->restrictToRange(-1, -1);});
    };
    auto flameGraphView = new Internal::FlameGraphView(d->modelManager, parent);
    prepareEventsView(flameGraphView);
    auto statisticsView = new Internal::QmlProfilerStatisticsView(d->modelManager, parent);
    prepareEventsView(statisticsView);
    auto quick3DView = new Internal::Quick3DFrameView(d->modelManager, parent);
    prepareEventsView(quick3DView);
    return { traceView, flameGraphView, statisticsView, quick3DView };
}

QString QmlProfilerPlainViewManager::fileDialogTraceFilesFilter()
{
    return Internal::QmlProfilerTool::fileDialogTraceFilesFilter();
}

QFuture<void> QmlProfilerPlainViewManager::loadTraceFile(const FilePath &file)
{
    return d->modelManager->load(file.toFSPathString());
}

void QmlProfilerPlainViewManager::clear()
{
    d->modelManager->clearAll();
}

milliseconds QmlProfilerPlainViewManager::traceDuration() const
{
    return duration_cast<milliseconds>(nanoseconds{d->modelManager->traceDuration()});
}

} // namespace QmlProfiler
