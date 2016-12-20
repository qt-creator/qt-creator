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
#include "qmlprofilerstatemanager.h"
#include "qmlprofilermodelmanager.h"
#include "qmlprofilerstatewidget.h"

#include <coreplugin/icore.h>
#include <utils/qtcassert.h>
#include <debugger/analyzer/analyzermanager.h>
#include <extensionsystem/pluginmanager.h>

#include <QDockWidget>

using namespace Debugger;
using namespace Utils;

namespace QmlProfiler {
namespace Internal {

class QmlProfilerViewManager::QmlProfilerViewManagerPrivate {
public:
    QmlProfilerTraceView *traceView;
    QmlProfilerStatisticsView *statisticsView;
    FlameGraphView *flameGraphView;
    QmlProfilerStateManager *profilerState;
    QmlProfilerModelManager *profilerModelManager;
};

QmlProfilerViewManager::QmlProfilerViewManager(QObject *parent,
                                               QmlProfilerModelManager *modelManager,
                                               QmlProfilerStateManager *profilerState)
    : QObject(parent), d(new QmlProfilerViewManagerPrivate)
{
    setObjectName(QLatin1String("QML Profiler View Manager"));
    d->traceView = nullptr;
    d->statisticsView = nullptr;
    d->flameGraphView = nullptr;
    d->profilerState = profilerState;
    d->profilerModelManager = modelManager;
    createViews();
}

QmlProfilerViewManager::~QmlProfilerViewManager()
{
    delete d;
}

void QmlProfilerViewManager::createViews()
{
    QTC_ASSERT(d->profilerModelManager, return);
    QTC_ASSERT(d->profilerState, return);

    d->traceView = new QmlProfilerTraceView(0, this, d->profilerModelManager);
    connect(d->traceView, &QmlProfilerTraceView::gotoSourceLocation,
            this, &QmlProfilerViewManager::gotoSourceLocation);
    connect(d->traceView, &QmlProfilerTraceView::typeSelected,
            this, &QmlProfilerViewManager::typeSelected);
    connect(this, &QmlProfilerViewManager::typeSelected,
            d->traceView, &QmlProfilerTraceView::selectByTypeId);

    new QmlProfilerStateWidget(d->profilerState, d->profilerModelManager, d->traceView);

    auto perspective = new Utils::Perspective;
    perspective->setName(tr("QML Profiler"));

    auto prepareEventsView = [this](QmlProfilerEventsView *view) {
        connect(view, &QmlProfilerEventsView::typeSelected,
                this, &QmlProfilerViewManager::typeSelected);
        connect(this, &QmlProfilerViewManager::typeSelected,
                view, &QmlProfilerEventsView::selectByTypeId);
        connect(d->profilerModelManager, &QmlProfilerModelManager::visibleFeaturesChanged,
                view, &QmlProfilerEventsView::onVisibleFeaturesChanged);
        connect(view, &QmlProfilerEventsView::gotoSourceLocation,
                this, &QmlProfilerViewManager::gotoSourceLocation);
        connect(view, &QmlProfilerEventsView::showFullRange,
                this, [this](){ d->profilerModelManager->restrictToRange(-1, -1);});
        new QmlProfilerStateWidget(d->profilerState, d->profilerModelManager, view);
    };

    d->statisticsView = new QmlProfilerStatisticsView(d->profilerModelManager);
    prepareEventsView(d->statisticsView);
    d->flameGraphView = new FlameGraphView(d->profilerModelManager);
    prepareEventsView(d->flameGraphView);

    QByteArray anchorDockId;
    if (d->traceView->isUsable()) {
        anchorDockId = d->traceView->objectName().toLatin1();
        perspective->addOperation({anchorDockId, d->traceView, {}, Perspective::SplitVertical});
        perspective->addOperation({d->flameGraphView->objectName().toLatin1(), d->flameGraphView,
                                   anchorDockId, Perspective::AddToTab});
    } else {
        anchorDockId = d->flameGraphView->objectName().toLatin1();
        perspective->addOperation({anchorDockId, d->flameGraphView, {},
                                   Perspective::SplitVertical});
    }
    perspective->addOperation({d->statisticsView->objectName().toLatin1(), d->statisticsView,
                               anchorDockId, Perspective::AddToTab});
    perspective->addOperation({anchorDockId, 0, {}, Perspective::Raise});

    Debugger::registerPerspective(Constants::QmlProfilerPerspectiveId, perspective);
}

QmlProfilerTraceView *QmlProfilerViewManager::traceView() const
{
    return d->traceView;
}

QmlProfilerStatisticsView *QmlProfilerViewManager::statisticsView() const
{
    return d->statisticsView;
}

FlameGraphView *QmlProfilerViewManager::flameGraphView() const
{
    return d->flameGraphView;
}

void QmlProfilerViewManager::clear()
{
    d->traceView->clear();
    d->flameGraphView->clear();
    d->statisticsView->clear();
}

} // namespace Internal
} // namespace QmlProfiler
