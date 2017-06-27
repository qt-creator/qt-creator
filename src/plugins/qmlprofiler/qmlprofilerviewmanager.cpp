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

    m_traceView = new QmlProfilerTraceView(0, this, m_profilerModelManager);
    connect(m_traceView, &QmlProfilerTraceView::gotoSourceLocation,
            this, &QmlProfilerViewManager::gotoSourceLocation);
    connect(m_traceView, &QmlProfilerTraceView::typeSelected,
            this, &QmlProfilerViewManager::typeSelected);
    connect(this, &QmlProfilerViewManager::typeSelected,
            m_traceView, &QmlProfilerTraceView::selectByTypeId);

    new QmlProfilerStateWidget(m_profilerState, m_profilerModelManager, m_traceView);

    auto perspective = new Utils::Perspective;
    perspective->setName(tr("QML Profiler"));

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

    QByteArray anchorDockId;
    if (m_traceView->isUsable()) {
        anchorDockId = m_traceView->objectName().toLatin1();
        perspective->addOperation({anchorDockId, m_traceView, {}, Perspective::SplitVertical});
        perspective->addOperation({m_flameGraphView->objectName().toLatin1(), m_flameGraphView,
                                   anchorDockId, Perspective::AddToTab});
    } else {
        anchorDockId = m_flameGraphView->objectName().toLatin1();
        perspective->addOperation({anchorDockId, m_flameGraphView, {},
                                   Perspective::SplitVertical});
    }
    perspective->addOperation({m_statisticsView->objectName().toLatin1(), m_statisticsView,
                               anchorDockId, Perspective::AddToTab});
    perspective->addOperation({anchorDockId, 0, {}, Perspective::Raise});

    Debugger::registerPerspective(Constants::QmlProfilerPerspectiveId, perspective);
}

void QmlProfilerViewManager::clear()
{
    m_traceView->clear();
    m_flameGraphView->clear();
    m_statisticsView->clear();
}

} // namespace Internal
} // namespace QmlProfiler
