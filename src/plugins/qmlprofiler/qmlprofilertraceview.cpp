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

#include "qmlprofilertraceview.h"
#include "qmlprofilertool.h"
#include "qmlprofilerstatemanager.h"
#include "qmlprofilermodelmanager.h"
#include "qmlprofilernotesmodel.h"
#include "qmlprofileranimationsmodel.h"
#include "qmlprofilerrangemodel.h"
#include "qmlprofilerplugin.h"

// Communication with the other views (limit events to range)
#include "qmlprofilerviewmanager.h"

#include "timeline/timelinezoomcontrol.h"
#include "timeline/timelinemodelaggregator.h"
#include "timeline/timelinerenderer.h"
#include "timeline/timelineoverviewrenderer.h"

// Needed for the load&save actions in the context menu
#include <analyzerbase/ianalyzertool.h>

#include <utils/styledbar.h>

#include <QQmlContext>
#include <QToolButton>
#include <QEvent>
#include <QVBoxLayout>
#include <QGraphicsObject>
#include <QScrollBar>
#include <QSlider>
#include <QMenu>
#include <QQuickItem>
#include <QQuickWidget>
#include <QApplication>

#include <math.h>

using namespace QmlDebug;

namespace QmlProfiler {
namespace Internal {

/////////////////////////////////////////////////////////
class QmlProfilerTraceView::QmlProfilerTraceViewPrivate
{
public:
    QmlProfilerTraceViewPrivate(QmlProfilerTraceView *qq) : q(qq) {}
    QmlProfilerTraceView *q;

    QmlProfilerTool *m_profilerTool;
    QmlProfilerViewManager *m_viewContainer;

    QSize m_sizeHint;

    QQuickWidget *m_mainView;
    QmlProfilerModelManager *m_modelManager;
    Timeline::TimelineModelAggregator *m_modelProxy;


    Timeline::TimelineZoomControl *m_zoomControl;
};

QmlProfilerTraceView::QmlProfilerTraceView(QWidget *parent, QmlProfilerTool *profilerTool, QmlProfilerViewManager *container, QmlProfilerModelManager *modelManager)
    : QWidget(parent), d(new QmlProfilerTraceViewPrivate(this))
{
    setObjectName(QLatin1String("QML Profiler"));

    d->m_zoomControl = new Timeline::TimelineZoomControl(this);
    connect(modelManager->traceTime(), &QmlProfilerTraceTime::timeChanged,
            d->m_zoomControl, &Timeline::TimelineZoomControl::setTrace);

    QVBoxLayout *groupLayout = new QVBoxLayout;
    groupLayout->setContentsMargins(0, 0, 0, 0);
    groupLayout->setSpacing(0);

    qmlRegisterType<Timeline::TimelineRenderer>("TimelineRenderer", 1, 0, "TimelineRenderer");
    qmlRegisterType<Timeline::TimelineOverviewRenderer>("TimelineOverviewRenderer", 1, 0,
                                                        "TimelineOverviewRenderer");
    qmlRegisterType<Timeline::TimelineZoomControl>();
    qmlRegisterType<Timeline::TimelineModel>();
    qmlRegisterType<Timeline::TimelineNotesModel>();

    d->m_mainView = new QQuickWidget(this);
    d->m_mainView->setResizeMode(QQuickWidget::SizeRootObjectToView);
    d->m_mainView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    groupLayout->addWidget(d->m_mainView);
    setLayout(groupLayout);

    d->m_profilerTool = profilerTool;
    d->m_viewContainer = container;

    d->m_modelProxy = new Timeline::TimelineModelAggregator(modelManager->notesModel(), this);
    d->m_modelManager = modelManager;

    connect(qobject_cast<QmlProfilerTool *>(profilerTool), &QmlProfilerTool::selectTimelineElement,
            this, &QmlProfilerTraceView::selectByEventIndex);
    connect(modelManager,SIGNAL(dataAvailable()), d->m_modelProxy,SIGNAL(dataAvailable()));

    // external models pushed on top
    foreach (QmlProfilerTimelineModel *timelineModel,
             QmlProfilerPlugin::instance->getModels(modelManager)) {
        d->m_modelProxy->addModel(timelineModel);
    }

    d->m_modelProxy->addModel(new QmlProfilerAnimationsModel(modelManager, d->m_modelProxy));

    for (int i = 0; i < MaximumRangeType; ++i)
        d->m_modelProxy->addModel(new QmlProfilerRangeModel(modelManager, (RangeType)i,
                                                            d->m_modelProxy));

    // Connect this last so that it's executed after the models have updated their data.
    connect(modelManager->qmlModel(), SIGNAL(changed()), d->m_modelProxy, SIGNAL(stateChanged()));

    // Minimum height: 5 rows of 20 pixels + scrollbar of 50 pixels + 20 pixels margin
    setMinimumHeight(170);

    d->m_mainView->rootContext()->setContextProperty(QLatin1String("timelineModelAggregator"),
                                                     d->m_modelProxy);
    d->m_mainView->rootContext()->setContextProperty(QLatin1String("zoomControl"),
                                                     d->m_zoomControl);
    d->m_mainView->setSource(QUrl(QLatin1String("qrc:/timeline/MainView.qml")));

    QQuickItem *rootObject = d->m_mainView->rootObject();
    connect(rootObject, SIGNAL(updateCursorPosition()), this, SLOT(updateCursorPosition()));
}

QmlProfilerTraceView::~QmlProfilerTraceView()
{
    delete d->m_mainView;
    delete d;
}

/////////////////////////////////////////////////////////
bool QmlProfilerTraceView::hasValidSelection() const
{
    QQuickItem *rootObject = d->m_mainView->rootObject();
    if (rootObject)
        return rootObject->property("selectionRangeReady").toBool();
    return false;
}

qint64 QmlProfilerTraceView::selectionStart() const
{
    return d->m_zoomControl->selectionStart();
}

qint64 QmlProfilerTraceView::selectionEnd() const
{
    return d->m_zoomControl->selectionEnd();
}

void QmlProfilerTraceView::clear()
{
    QMetaObject::invokeMethod(d->m_mainView->rootObject(), "clear");
}

void QmlProfilerTraceView::selectByTypeId(int typeId)
{
    QQuickItem *rootObject = d->m_mainView->rootObject();
    if (!rootObject)
        return;
    QMetaObject::invokeMethod(rootObject, "selectByTypeId", Q_ARG(QVariant,QVariant(typeId)));
}

void QmlProfilerTraceView::selectBySourceLocation(const QString &filename, int line, int column)
{
    QQuickItem *rootObject = d->m_mainView->rootObject();
    if (!rootObject)
        return;

    for (int modelIndex = 0; modelIndex < d->m_modelProxy->modelCount(); ++modelIndex) {
        int typeId = d->m_modelProxy->model(modelIndex)->selectionIdForLocation(filename, line,
                                                                                column);
        if (typeId != -1) {
            QMetaObject::invokeMethod(rootObject, "selectBySelectionId",
                                      Q_ARG(QVariant,QVariant(modelIndex)),
                                      Q_ARG(QVariant,QVariant(typeId)));
            return;
        }
    }
}

void QmlProfilerTraceView::selectByEventIndex(int modelId, int eventIndex)
{
    QQuickItem *rootObject = d->m_mainView->rootObject();
    if (!rootObject)
        return;

    const int modelIndex = d->m_modelProxy->modelIndexById(modelId);
    QTC_ASSERT(modelIndex != -1, return);
    QMetaObject::invokeMethod(rootObject, "selectByIndices",
                              Q_ARG(QVariant, QVariant(modelIndex)),
                              Q_ARG(QVariant, QVariant(eventIndex)));
}

/////////////////////////////////////////////////////////
// Goto source location
void QmlProfilerTraceView::updateCursorPosition()
{
    QQuickItem *rootObject = d->m_mainView->rootObject();
    QString file = rootObject->property("fileName").toString();
    if (!file.isEmpty())
        emit gotoSourceLocation(file, rootObject->property("lineNumber").toInt(),
                                rootObject->property("columnNumber").toInt());

    emit typeSelected(rootObject->property("typeId").toInt());
}

void QmlProfilerTraceView::mousePressEvent(QMouseEvent *event)
{
    d->m_zoomControl->setWindowLocked(true);
    QWidget::mousePressEvent(event);
}

void QmlProfilerTraceView::mouseReleaseEvent(QMouseEvent *event)
{
    d->m_zoomControl->setWindowLocked(false);
    QWidget::mouseReleaseEvent(event);
}

////////////////////////////////////////////////////////////////
// Context menu
void QmlProfilerTraceView::contextMenuEvent(QContextMenuEvent *ev)
{
    showContextMenu(ev->globalPos());
}

void QmlProfilerTraceView::showContextMenu(QPoint position)
{
    QMenu menu;
    QAction *viewAllAction = 0;

    QmlProfilerTool *profilerTool = qobject_cast<QmlProfilerTool *>(d->m_profilerTool);

    if (profilerTool)
        menu.addActions(profilerTool->profilerContextMenuActions());

    menu.addSeparator();

    QAction *getLocalStatsAction = menu.addAction(tr("Limit Events Pane to Current Range"));
    if (!d->m_viewContainer->hasValidSelection())
        getLocalStatsAction->setEnabled(false);

    QAction *getGlobalStatsAction = menu.addAction(tr("Show Full Range in Events Pane"));
    if (d->m_viewContainer->hasGlobalStats())
        getGlobalStatsAction->setEnabled(false);

    if (d->m_zoomControl->traceDuration() > 0) {
        menu.addSeparator();
        viewAllAction = menu.addAction(tr("Reset Zoom"));
    }

    QAction *selectedAction = menu.exec(position);

    if (selectedAction) {
        if (selectedAction == viewAllAction) {
            d->m_zoomControl->setRange(d->m_zoomControl->traceStart(),
                                       d->m_zoomControl->traceEnd());
        }
        if (selectedAction == getLocalStatsAction) {
            d->m_viewContainer->getStatisticsInRange(
                        d->m_viewContainer->selectionStart(),
                        d->m_viewContainer->selectionEnd());
        }
        if (selectedAction == getGlobalStatsAction)
            d->m_viewContainer->getStatisticsInRange(-1, -1);
    }
}

////////////////////////////////////////////////////////////////

void QmlProfilerTraceView::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::EnabledChange) {
        QQuickItem *rootObject = d->m_mainView->rootObject();
        rootObject->setProperty("enabled", isEnabled());
    }
}

} // namespace Internal
} // namespace QmlProfiler
