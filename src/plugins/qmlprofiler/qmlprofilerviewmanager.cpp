// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlprofilerconstants.h"
#include "qmlprofilerstatewidget.h"
#include "qmlprofilertr.h"
#include "qmlprofilerviewmanager.h"

#include <debugger/analyzer/analyzermanager.h>

#include <projectexplorer/projectexplorerconstants.h>

#include <utils/qtcassert.h>

#include <QDockWidget>

using namespace Debugger;
using namespace Utils;

namespace QmlProfiler {
namespace Internal {

QmlProfilerViewManager::QmlProfilerViewManager(QObject *parent,
                                               QmlProfilerModelManager *modelManager,
                                               QmlProfilerStateManager *profilerState)
    : QObject(parent)
{
    setObjectName("QML Profiler View Manager");
    m_profilerState = profilerState;
    m_profilerModelManager = modelManager;

    QTC_ASSERT(m_profilerModelManager, return);
    QTC_ASSERT(m_profilerState, return);

    m_perspective = new Perspective(Constants::QmlProfilerPerspectiveId, Tr::tr("QML Profiler"));
    m_perspective->setAboutToActivateCallback([this]() { createViews(); });
}

void QmlProfilerViewManager::createViews()
{
    m_traceView = new QmlProfilerTraceView(nullptr, this, m_profilerModelManager);
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

    QWidget *anchorDock = nullptr;
    if (m_traceView->isUsable()) {
        anchorDock = m_traceView;
        m_perspective->addWindow(m_traceView, Perspective::SplitVertical, nullptr);
        m_perspective->addWindow(m_flameGraphView, Perspective::AddToTab, m_traceView);
        m_perspective->addWindow(m_quick3dView, Perspective::AddToTab, m_flameGraphView);
    } else {
        anchorDock = m_flameGraphView;
        m_perspective->addWindow(m_flameGraphView, Perspective::SplitVertical, nullptr);
        m_perspective->addWindow(m_quick3dView, Perspective::AddToTab, m_flameGraphView);
    }
    m_perspective->addWindow(m_statisticsView, Perspective::AddToTab, anchorDock);
    m_perspective->addWindow(anchorDock, Perspective::Raise, nullptr);
    m_perspective->setAboutToActivateCallback(Perspective::Callback());
    emit viewsCreated();
}

QmlProfilerViewManager::~QmlProfilerViewManager()
{
    delete m_traceView;
    delete m_flameGraphView;
    delete m_statisticsView;
    delete m_quick3dView;
    delete m_perspective;
}

void QmlProfilerViewManager::clear()
{
    if (m_traceView)
        m_traceView->clear();
}

} // namespace Internal
} // namespace QmlProfiler
