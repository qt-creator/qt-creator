/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qmlprofilerviewmanager.h"

#include "qmlprofilertraceview.h"
#include "qmlprofilereventview.h"
#include "qmlprofilertool.h"
#include "qmlprofilerstatemanager.h"
#include "qmlprofilermodelmanager.h"
#include "qmlprofilerstatewidget.h"

#include <utils/qtcassert.h>
#include <utils/fancymainwindow.h>
#include <analyzerbase/analyzermanager.h>

#include <QDockWidget>

using namespace Analyzer;

namespace QmlProfiler {
namespace Internal {

class QmlProfilerViewManager::QmlProfilerViewManagerPrivate {
public:
    QmlProfilerViewManagerPrivate(QmlProfilerViewManager *qq) { Q_UNUSED(qq); }

    QDockWidget *timelineDock;
    QmlProfilerTraceView *traceView;
    QmlProfilerEventsWidget *eventsView;
    QmlProfilerStateManager *profilerState;
    QmlProfilerModelManager *profilerModelManager;
    QmlProfilerTool *profilerTool;
};

QmlProfilerViewManager::QmlProfilerViewManager(QObject *parent,
                                               QmlProfilerTool *profilerTool,
                                               QmlProfilerModelManager *modelManager,
                                               QmlProfilerStateManager *profilerState)
    : QObject(parent), d(new QmlProfilerViewManagerPrivate(this))
{
    setObjectName(QLatin1String("QML Profiler View Manager"));
    d->traceView = 0;
    d->eventsView = 0;
    d->profilerState = profilerState;
    d->profilerModelManager = modelManager;
    d->profilerTool = profilerTool;
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

    Utils::FancyMainWindow *mw = AnalyzerManager::mainWindow();

    d->traceView = new QmlProfilerTraceView(mw,
                                            d->profilerTool,
                                            this,
                                            d->profilerModelManager);
    d->traceView->setWindowTitle(tr("Timeline"));
    connect(d->traceView, SIGNAL(gotoSourceLocation(QString,int,int)),
            this, SIGNAL(gotoSourceLocation(QString,int,int)));

    d->eventsView = new QmlProfilerEventsWidget(mw, d->profilerTool, this,
                                                d->profilerModelManager);
    d->eventsView->setWindowTitle(tr("Events"));
    connect(d->eventsView, SIGNAL(gotoSourceLocation(QString,int,int)), this,
            SIGNAL(gotoSourceLocation(QString,int,int)));
    connect(d->eventsView, SIGNAL(typeSelected(int)), d->traceView, SLOT(selectByTypeId(int)));
    connect(d->traceView, SIGNAL(typeSelected(int)), d->eventsView, SLOT(selectByTypeId(int)));
    connect(d->profilerModelManager, &QmlProfilerModelManager::visibleFeaturesChanged,
            d->eventsView, &QmlProfilerEventsWidget::onVisibleFeaturesChanged);

    QDockWidget *eventsDock = AnalyzerManager::createDockWidget
            (QmlProfilerToolId, d->eventsView);
    d->timelineDock = AnalyzerManager::createDockWidget
            (QmlProfilerToolId, d->traceView);

    eventsDock->show();
    d->timelineDock->show();

    mw->splitDockWidget(mw->toolBarDockWidget(), d->timelineDock, Qt::Vertical);
    mw->tabifyDockWidget(d->timelineDock, eventsDock);
    d->timelineDock->raise();

    new QmlProfilerStateWidget(d->profilerState, d->profilerModelManager, d->eventsView);
    new QmlProfilerStateWidget(d->profilerState, d->profilerModelManager, d->traceView);
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

bool QmlProfilerViewManager::hasGlobalStats() const
{
    return d->eventsView->hasGlobalStats();
}

void QmlProfilerViewManager::getStatisticsInRange(qint64 rangeStart, qint64 rangeEnd)
{
    d->eventsView->getStatisticsInRange(rangeStart, rangeEnd);
}

void QmlProfilerViewManager::raiseTimeline()
{
    d->timelineDock->raise();
    d->traceView->setFocus();
}

void QmlProfilerViewManager::clear()
{
    d->traceView->clear();
    d->eventsView->clear();
}

} // namespace Internal
} // namespace QmlProfiler
