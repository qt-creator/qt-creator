// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlprofilerconstants.h"
#include "qmlprofilerstatewidget.h"
#include "qmlprofilertr.h"
#include "qmlprofilerviewmanager.h"

#include <coreplugin/perspective.h>

#include <projectexplorer/projectexplorerconstants.h>

using namespace Core;
using namespace Utils;

namespace QmlProfiler::Internal {

QmlProfilerViewManager::QmlProfilerViewManager(QmlProfilerModelManager *modelManager,
                                               QmlProfilerStateManager *profilerState)
    : m_profilerModelManager(modelManager)
    , m_profilerState(profilerState)
    , m_traceView(nullptr, modelManager)
    , m_statisticsView(modelManager)
    , m_flameGraphView(modelManager)
    , m_quick3dView(modelManager)
    , m_perspective(Constants::QmlProfilerPerspectiveId, Tr::tr("QML Profiler"))
{
    setObjectName("QML Profiler View Manager");

    connect(&m_traceView, &QmlProfilerTraceView::gotoSourceLocation,
            this, &QmlProfilerViewManager::gotoSourceLocation);
    connect(&m_traceView, &QmlProfilerTraceView::typeSelected,
            this, &QmlProfilerViewManager::typeSelected);
    connect(this, &QmlProfilerViewManager::typeSelected,
            &m_traceView, &QmlProfilerTraceView::selectByTypeId);

    new QmlProfilerStateWidget(m_profilerState, m_profilerModelManager, &m_traceView);

    auto prepareEventsView = [this](QmlProfilerEventsView *view) {
        connect(view, &QmlProfilerEventsView::typeSelected,
                this, &QmlProfilerViewManager::typeSelected);
        connect(this, &QmlProfilerViewManager::typeSelected,
                view, &QmlProfilerEventsView::selectByTypeId);
        connect(m_profilerModelManager, &QmlProfilerModelManager::visibleFeaturesChanged,
                view, &QmlProfilerEventsView::onVisibleFeaturesChanged);
        connect(view, &QmlProfilerEventsView::gotoSourceLocation,
                this, &QmlProfilerViewManager::gotoSourceLocation);
        connect(view, &QmlProfilerEventsView::showFullRange,
                this, [this](){ m_profilerModelManager->restrictToRange(-1, -1);});
        new QmlProfilerStateWidget(m_profilerState, m_profilerModelManager, view);
    };

    prepareEventsView(&m_statisticsView);
    prepareEventsView(&m_flameGraphView);
    prepareEventsView(&m_quick3dView);

    m_perspective.addWindow(&m_traceView, Perspective::SplitVertical, nullptr);
    m_perspective.addWindow(&m_flameGraphView, Perspective::AddToTab, &m_traceView);
    m_perspective.addWindow(&m_quick3dView, Perspective::AddToTab, &m_flameGraphView);
    m_perspective.addWindow(&m_statisticsView, Perspective::AddToTab, &m_traceView);
    m_perspective.addWindow(&m_traceView, Perspective::Raise, nullptr);
}

void QmlProfilerViewManager::clear()
{
    m_traceView.clear();
}

} // namespace QmlProfiler::Internal
