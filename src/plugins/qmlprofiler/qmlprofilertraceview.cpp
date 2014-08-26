/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qmlprofilertraceview.h"
#include "qmlprofilertool.h"
#include "qmlprofilerstatemanager.h"
#include "qmlprofilermodelmanager.h"
#include "qmlprofilertimelinemodelproxy.h"
#include "timelinemodelaggregator.h"

// Needed for the load&save actions in the context menu
#include <analyzerbase/ianalyzertool.h>

// Communication with the other views (limit events to range)
#include "qmlprofilerviewmanager.h"

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
#include <QApplication>

#include <math.h>

using namespace QmlDebug;

namespace QmlProfiler {
namespace Internal {

/////////////////////////////////////////////////////////
ZoomControl::ZoomControl(const QmlProfilerTraceTime *traceTime, QObject *parent) :
    QObject(parent), m_startTime(traceTime->startTime()), m_endTime(traceTime->endTime()),
    m_windowStart(traceTime->startTime()), m_windowEnd(traceTime->endTime()),
    m_traceTime(traceTime), m_windowLocked(false)
{
    connect(traceTime, SIGNAL(startTimeChanged(qint64)), this, SLOT(rebuildWindow()));
    connect(traceTime, SIGNAL(endTimeChanged(qint64)), this, SLOT(rebuildWindow()));
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(moveWindow()));
}

void ZoomControl::setRange(qint64 startTime, qint64 endTime)
{
    if (m_startTime != startTime || m_endTime != endTime) {
        m_timer.stop();
        m_startTime = startTime;
        m_endTime = endTime;
        rebuildWindow();
        emit rangeChanged();
    }
}

void ZoomControl::rebuildWindow()
{
    qint64 minDuration = 1; // qMax needs equal data types, so literal 1 won't do
    qint64 shownDuration = qMax(duration(), minDuration);

    qint64 oldWindowStart = m_windowStart;
    qint64 oldWindowEnd = m_windowEnd;
    if (m_traceTime->duration() / shownDuration < MAX_ZOOM_FACTOR) {
        m_windowStart = m_traceTime->startTime();
        m_windowEnd = m_traceTime->endTime();
    } else if (windowLength() / shownDuration > MAX_ZOOM_FACTOR ||
               windowLength() / shownDuration * 2 < MAX_ZOOM_FACTOR) {
        qint64 keep = shownDuration * MAX_ZOOM_FACTOR / 2 - shownDuration;
        m_windowStart = m_startTime - keep;
        if (m_windowStart < m_traceTime->startTime()) {
            keep += m_traceTime->startTime() - m_windowStart;
            m_windowStart = m_traceTime->startTime();
        }

        m_windowEnd = m_endTime + keep;
        if (m_windowEnd > m_traceTime->endTime()) {
            m_windowStart = qMax(m_traceTime->startTime(),
                                 m_windowStart - m_windowEnd - m_traceTime->endTime());
            m_windowEnd = m_traceTime->endTime();
        }
    } else {
        m_timer.start(500);
    }
    if (oldWindowStart != m_windowStart || oldWindowEnd != m_windowEnd)
        emit windowChanged();
}

void ZoomControl::moveWindow()
{
    if (m_windowLocked)
        return;
    m_timer.stop();

    qint64 offset = (m_endTime - m_windowEnd + m_startTime - m_windowStart) / 2;
    if (offset == 0 || (offset < 0 && m_windowStart == m_traceTime->startTime()) ||
            (offset > 0 && m_windowEnd == m_traceTime->endTime())) {
        return;
    } else if (offset > duration()) {
        offset = (offset + duration()) / 2;
    } else if (offset < -duration()) {
        offset = (offset - duration()) / 2;
    }
    m_windowStart += offset;
    if (m_windowStart < m_traceTime->startTime()) {
        m_windowEnd += m_traceTime->startTime() - m_windowStart;
        m_windowStart = m_traceTime->startTime();
    }
    m_windowEnd += offset;
    if (m_windowEnd > m_traceTime->endTime()) {
        m_windowStart -= m_windowEnd - m_traceTime->endTime();
        m_windowEnd = m_traceTime->endTime();
    }
    emit windowChanged();
    m_timer.start(100);
}

/////////////////////////////////////////////////////////
class QmlProfilerTraceView::QmlProfilerTraceViewPrivate
{
public:
    QmlProfilerTraceViewPrivate(QmlProfilerTraceView *qq) : q(qq) {}
    ~QmlProfilerTraceViewPrivate()
    {
        delete m_mainView;
    }

    QmlProfilerTraceView *q;

    QmlProfilerStateManager *m_profilerState;
    Analyzer::IAnalyzerTool *m_profilerTool;
    QmlProfilerViewManager *m_viewContainer;

    QSize m_sizeHint;

    QQuickView *m_mainView;
    QmlProfilerModelManager *m_modelManager;
    TimelineModelAggregator *m_modelProxy;


    ZoomControl *m_zoomControl;
};

QmlProfilerTraceView::QmlProfilerTraceView(QWidget *parent, Analyzer::IAnalyzerTool *profilerTool, QmlProfilerViewManager *container, QmlProfilerModelManager *modelManager, QmlProfilerStateManager *profilerState)
    : QWidget(parent), d(new QmlProfilerTraceViewPrivate(this))
{
    setObjectName(QLatin1String("QML Profiler"));

    d->m_zoomControl = new ZoomControl(modelManager->traceTime(), this);
    connect(d->m_zoomControl, SIGNAL(rangeChanged()), this, SLOT(updateRange()));

    QVBoxLayout *groupLayout = new QVBoxLayout;
    groupLayout->setContentsMargins(0, 0, 0, 0);
    groupLayout->setSpacing(0);

    d->m_mainView = new QmlProfilerQuickView(this);
    d->m_mainView->setResizeMode(QQuickView::SizeRootObjectToView);
    QWidget *mainViewContainer = QWidget::createWindowContainer(d->m_mainView);
    mainViewContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    enableToolbar(false);

    groupLayout->addWidget(mainViewContainer);
    setLayout(groupLayout);

    d->m_profilerTool = profilerTool;
    d->m_viewContainer = container;
    d->m_modelManager = modelManager;
    d->m_modelProxy = new TimelineModelAggregator(this);
    d->m_modelProxy->setModelManager(modelManager);
    connect(d->m_modelManager, SIGNAL(stateChanged()),
            this, SLOT(profilerDataModelStateChanged()));
    d->m_mainView->rootContext()->setContextProperty(QLatin1String("qmlProfilerModelProxy"),
                                                     d->m_modelProxy);
    d->m_profilerState = profilerState;

    // Minimum height: 5 rows of 20 pixels + scrollbar of 50 pixels + 20 pixels margin
    setMinimumHeight(170);
}

QmlProfilerTraceView::~QmlProfilerTraceView()
{
    delete d;
}

/////////////////////////////////////////////////////////
// Initialize widgets
void QmlProfilerTraceView::reset()
{
    d->m_mainView->rootContext()->setContextProperty(QLatin1String("zoomControl"), d->m_zoomControl);
    d->m_mainView->setSource(QUrl(QLatin1String("qrc:/qmlprofiler/MainView.qml")));

    QQuickItem *rootObject = d->m_mainView->rootObject();
    connect(rootObject, SIGNAL(updateCursorPosition()), this, SLOT(updateCursorPosition()));
}

/////////////////////////////////////////////////////////
bool QmlProfilerTraceView::hasValidSelection() const
{
    QQuickItem *rootObject = d->m_mainView->rootObject();
    if (rootObject)
        return rootObject->property("selectionRangeReady").toBool();
    return false;
}

void QmlProfilerTraceView::enableToolbar(bool enable)
{
    QMetaObject::invokeMethod(d->m_mainView->rootObject(), "enableButtonsBar",
                              Q_ARG(QVariant,QVariant(enable)));
}

qint64 QmlProfilerTraceView::selectionStart() const
{
    QQuickItem *rootObject = d->m_mainView->rootObject();
    if (rootObject)
        return rootObject->property("selectionRangeStart").toLongLong();
    return 0;
}

qint64 QmlProfilerTraceView::selectionEnd() const
{
    QQuickItem *rootObject = d->m_mainView->rootObject();
    if (rootObject)
        return rootObject->property("selectionRangeEnd").toLongLong();
    return 0;
}

void QmlProfilerTraceView::clear()
{
    QMetaObject::invokeMethod(d->m_mainView->rootObject(), "clear");
}

void QmlProfilerTraceView::selectByTypeIndex(int typeIndex)
{
    QQuickItem *rootObject = d->m_mainView->rootObject();
    if (!rootObject)
        return;

    for (int modelIndex = 0; modelIndex < d->m_modelProxy->modelCount(); ++modelIndex) {
        int eventId = d->m_modelProxy->eventIdForTypeIndex(modelIndex, typeIndex);
        if (eventId != -1) {
            QMetaObject::invokeMethod(rootObject, "selectById",
                                      Q_ARG(QVariant,QVariant(modelIndex)),
                                      Q_ARG(QVariant,QVariant(eventId)));
            return;
        }
    }
}

void QmlProfilerTraceView::selectBySourceLocation(const QString &filename, int line, int column)
{
    QQuickItem *rootObject = d->m_mainView->rootObject();
    if (!rootObject)
        return;

    for (int modelIndex = 0; modelIndex < d->m_modelProxy->modelCount(); ++modelIndex) {
        int eventId = d->m_modelProxy->eventIdForLocation(modelIndex, filename, line, column);
        if (eventId != -1) {
            QMetaObject::invokeMethod(rootObject, "selectById",
                                      Q_ARG(QVariant,QVariant(modelIndex)),
                                      Q_ARG(QVariant,QVariant(eventId)));
            return;
        }
    }
}

/////////////////////////////////////////////////////////
// Goto source location
void QmlProfilerTraceView::updateCursorPosition()
{
    QQuickItem *rootObject = d->m_mainView->rootObject();
    emit gotoSourceLocation(rootObject->property("fileName").toString(),
                            rootObject->property("lineNumber").toInt(),
                            rootObject->property("columnNumber").toInt());
}

////////////////////////////////////////////////////////
// Zoom control
void QmlProfilerTraceView::updateRange()
{
    if (!d->m_modelManager)
        return;
    qreal duration = d->m_zoomControl->endTime() - d->m_zoomControl->startTime();
    if (duration <= 0)
        return;
    if (d->m_modelManager->traceTime()->duration() <= 0)
        return;
    QMetaObject::invokeMethod(d->m_mainView->rootObject()->findChild<QObject*>(QLatin1String("zoomSliderToolBar")), "updateZoomLevel");
}

////////////////////////////////////////////////////////
void QmlProfilerTraceView::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    emit resized();
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

    if (!d->m_modelProxy->isEmpty()) {
        menu.addSeparator();
        viewAllAction = menu.addAction(tr("Reset Zoom"));
    }

    QAction *selectedAction = menu.exec(position);

    if (selectedAction) {
        if (selectedAction == viewAllAction) {
            d->m_zoomControl->setRange(
                        d->m_modelManager->traceTime()->startTime(),
                        d->m_modelManager->traceTime()->endTime());
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
// Profiler State
void QmlProfilerTraceView::profilerDataModelStateChanged()
{
    switch (d->m_modelManager->state()) {
        case QmlProfilerDataState::Empty: break;
        case QmlProfilerDataState::ClearingData:
            d->m_mainView->hide();
            enableToolbar(false);
        break;
        case QmlProfilerDataState::AcquiringData: break;
        case QmlProfilerDataState::ProcessingData: break;
        case QmlProfilerDataState::Done:
            enableToolbar(true);
            d->m_mainView->show();
        break;
    default:
        break;
    }
}

bool QmlProfilerQuickView::event(QEvent *ev)
{
    // We assume context menus can only be triggered by mouse press, mouse release, or
    // pre-synthesized events from the window system.

    bool relayed = false;
    switch (ev->type()) {
    case QEvent::ContextMenu:
        // In the case of mouse clicks the active popup gets automatically closed before they show
        // up here. That's not necessarily the case with keyboard triggered context menu events, so
        // we just ignore them if there is a popup already. Also, the event's pos() and globalPos()
        // don't make much sense in this case, so we just put the menu in the upper left corner.
        if (QApplication::activePopupWidget() == 0) {
            ev->accept();
            parent->showContextMenu(parent->mapToGlobal(QPoint(0,0)));
            relayed = true;
        }
        break;
    case QEvent::MouseMove:
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease: {
        QMouseEvent *orig = static_cast<QMouseEvent *>(ev);
        QCoreApplication::instance()->postEvent(parent->window()->windowHandle(),
                new QMouseEvent(orig->type(), parent->window()->mapFromGlobal(orig->globalPos()),
                                orig->button(), orig->buttons(), orig->modifiers()));
        relayed = true;
        break;
    }
    default:
        break;
    }

    // QQuickView will eat mouse events even if they're not accepted by any QML construct. So we
    // ignore the return value of event() above.
    return QQuickView::event(ev) || relayed;
}

} // namespace Internal
} // namespace QmlProfiler
