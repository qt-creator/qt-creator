/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "qmlprofilerviewmanager.h"

#include "qmlprofilertool.h"
#include "qmlprofilerstatewidget.h"

#include <utils/qtcassert.h>
#include <debugger/analyzer/analyzermanager.h>

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

    m_traceView = new QmlProfilerTraceView(nullptr, this, m_profilerModelManager);
    connect(m_traceView, &QmlProfilerTraceView::gotoSourceLocation,
            this, &QmlProfilerViewManager::gotoSourceLocation);
    connect(m_traceView, &QmlProfilerTraceView::typeSelected,
            this, &QmlProfilerViewManager::typeSelected);
    connect(this, &QmlProfilerViewManager::typeSelected,
            m_traceView, &QmlProfilerTraceView::selectByTypeId);

    new QmlProfilerStateWidget(m_profilerState, m_profilerModelManager, m_traceView);

    m_perspective = new Utils::Perspective(Constants::QmlProfilerPerspectiveId, tr("QML Profiler"));

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

    QWidget *anchorDock = nullptr;
    if (m_traceView->isUsable()) {
        anchorDock = m_traceView;
        m_perspective->addWindow(m_traceView, Perspective::SplitVertical, nullptr);
        m_perspective->addWindow(m_flameGraphView, Perspective::AddToTab, m_traceView);
    } else {
        anchorDock = m_flameGraphView;
        m_perspective->addWindow(m_flameGraphView, Perspective::SplitVertical, nullptr);
    }
    m_perspective->addWindow(m_statisticsView, Perspective::AddToTab, anchorDock);
    m_perspective->addWindow(anchorDock, Perspective::Raise, nullptr);
}

QmlProfilerViewManager::~QmlProfilerViewManager()
{
    delete m_traceView;
    delete m_flameGraphView;
    delete m_statisticsView;
    delete m_perspective;
}

void QmlProfilerViewManager::clear()
{
    m_traceView->clear();
}

} // namespace Internal
} // namespace QmlProfiler
