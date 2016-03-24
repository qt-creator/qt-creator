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

#include "qmlprofilertraceview.h"
#include "qmlprofilerstatisticsview.h"
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
    QmlProfilerViewManagerPrivate(QmlProfilerViewManager *qq) { Q_UNUSED(qq); }

    QmlProfilerTraceView *traceView;
    QList<QmlProfilerEventsView *> eventsViews;
    QmlProfilerStateManager *profilerState;
    QmlProfilerModelManager *profilerModelManager;
    QmlProfilerEventsViewFactory *eventsViewFactory;
};

QmlProfilerViewManager::QmlProfilerViewManager(QObject *parent,
                                               QmlProfilerModelManager *modelManager,
                                               QmlProfilerStateManager *profilerState)
    : QObject(parent), d(new QmlProfilerViewManagerPrivate(this))
{
    setObjectName(QLatin1String("QML Profiler View Manager"));
    d->traceView = 0;
    d->profilerState = profilerState;
    d->profilerModelManager = modelManager;
    d->eventsViewFactory =
            ExtensionSystem::PluginManager::getObject<QmlProfilerEventsViewFactory>();
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
    d->traceView->setWindowTitle(tr("Timeline"));
    connect(d->traceView, &QmlProfilerTraceView::gotoSourceLocation,
            this, &QmlProfilerViewManager::gotoSourceLocation);
    connect(d->traceView, &QmlProfilerTraceView::typeSelected,
            this, &QmlProfilerViewManager::typeSelected);
    connect(this, &QmlProfilerViewManager::typeSelected,
            d->traceView, &QmlProfilerTraceView::selectByTypeId);

    new QmlProfilerStateWidget(d->profilerState, d->profilerModelManager, d->traceView);

    Utils::Perspective perspective;
    perspective.setName(tr("QML Profiler"));
    perspective.addOperation({Constants::QmlProfilerTimelineDockId, d->traceView, {},
                              Perspective::SplitVertical});

    d->eventsViews << new QmlProfilerStatisticsView(0, d->profilerModelManager);
    if (d->eventsViewFactory)
        d->eventsViews.append(d->eventsViewFactory->create(0, d->profilerModelManager));

    foreach (QmlProfilerEventsView *view, d->eventsViews) {
        connect(view, &QmlProfilerEventsView::typeSelected,
                this, &QmlProfilerViewManager::typeSelected);
        connect(this, &QmlProfilerViewManager::typeSelected,
                view, &QmlProfilerEventsView::selectByTypeId);
        connect(d->profilerModelManager, &QmlProfilerModelManager::visibleFeaturesChanged,
                view, &QmlProfilerEventsView::onVisibleFeaturesChanged);
        connect(view, &QmlProfilerEventsView::gotoSourceLocation,
                this, &QmlProfilerViewManager::gotoSourceLocation);
        connect(view, &QmlProfilerEventsView::showFullRange,
                this, [this](){restrictEventsToRange(-1, -1);});
        QByteArray dockId = view->objectName().toLatin1();
        perspective.addOperation({dockId, view, Constants::QmlProfilerTimelineDockId, Perspective::AddToTab});
        new QmlProfilerStateWidget(d->profilerState, d->profilerModelManager, view);
    }
    perspective.addOperation({Constants::QmlProfilerTimelineDockId, 0, {}, Perspective::Raise});
    Debugger::registerPerspective(Constants::QmlProfilerPerspectiveId, perspective);
}

bool QmlProfilerViewManager::hasValidSelection() const
{
    return d->traceView->hasValidSelection();
}

qint64 QmlProfilerViewManager::selectionStart() const
{
    return d->traceView->selectionStart();
}

qint64 QmlProfilerViewManager::selectionEnd() const
{
    return d->traceView->selectionEnd();
}

bool QmlProfilerViewManager::isEventsRestrictedToRange() const
{
    foreach (QmlProfilerEventsView *view, d->eventsViews) {
        if (view->isRestrictedToRange())
            return true;
    }
    return false;
}

void QmlProfilerViewManager::restrictEventsToRange(qint64 rangeStart, qint64 rangeEnd)
{
    foreach (QmlProfilerEventsView *view, d->eventsViews)
        view->restrictToRange(rangeStart, rangeEnd);
}

void QmlProfilerViewManager::raiseTimeline()
{
    QTC_ASSERT(qobject_cast<QDockWidget *>(d->traceView->parentWidget()), return);
    d->traceView->parentWidget()->raise();
    d->traceView->setFocus();
}

void QmlProfilerViewManager::clear()
{
    d->traceView->clear();
    foreach (QmlProfilerEventsView *view, d->eventsViews)
        view->clear();
}

} // namespace Internal
} // namespace QmlProfiler
