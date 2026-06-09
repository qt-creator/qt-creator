// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlprofilerconstants.h"
#include "qmlprofilerstatewidget.h"
#include "qmlprofilertr.h"
#include "qmlprofilerviewmanager.h"

#include <coreplugin/perspective.h>

#include <projectexplorer/projectexplorerconstants.h>

#include <utils/qtcassert.h>

using namespace Core;
using namespace Utils;

namespace QmlProfiler::Internal {

QmlProfilerViewManager::QmlProfilerViewManager(QmlProfilerModelManager *modelManager,
                                               QmlProfilerStateManager *profilerState)
{
    setObjectName("QML Profiler View Manager");
    m_profilerState = profilerState;
    m_profilerModelManager = modelManager;

    QTC_ASSERT(m_profilerModelManager, return);
    QTC_ASSERT(m_profilerState, return);

    m_perspective = new Perspective(Constants::QmlProfilerPerspectiveId, Tr::tr("QML Profiler"));

    m_traceView = new QmlProfilerTraceView(nullptr, m_profilerModelManager);
    connect(m_traceView, &QmlProfilerTraceView::gotoSourceLocation,
            this, &QmlProfilerViewManager::gotoSourceLocation);
    connect(m_traceView, &QmlProfilerTraceView::typeSelected,
            this, &QmlProfilerViewManager::typeSelected);
    connect(this, &QmlProfilerViewManager::typeSelected,
            m_traceView, &QmlProfilerTraceView::selectByTypeId);

    new QmlProfilerStateWidget(m_profilerState, m_profilerModelManager, m_traceView);

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

    m_statisticsView = new QmlProfilerStatisticsView(m_profilerModelManager);
    prepareEventsView(m_statisticsView);
    m_flameGraphView = new FlameGraphView(m_profilerModelManager);
    prepareEventsView(m_flameGraphView);
    m_quick3dView = new Quick3DFrameView(m_profilerModelManager);
    prepareEventsView(m_quick3dView);

    m_perspective->addWindow(m_traceView, Perspective::SplitVertical, nullptr);
    m_perspective->addWindow(m_flameGraphView, Perspective::AddToTab, m_traceView);
    m_perspective->addWindow(m_quick3dView, Perspective::AddToTab, m_flameGraphView);
    m_perspective->addWindow(m_statisticsView, Perspective::AddToTab, m_traceView);
    m_perspective->addWindow(m_traceView, Perspective::Raise, nullptr);
}

QmlProfilerViewManager::~QmlProfilerViewManager()
{
    // views are owned by m_perspective - deletion happens their
    delete m_perspective;
}

void QmlProfilerViewManager::clear()
{
    m_traceView->clear();
}

} // namespace QmlProfiler::Internal
