/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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
#include "qmlprofilerdatamodel.h"

// Needed for the load&save actions in the context menu
#include <analyzerbase/ianalyzertool.h>

// Comunication with the other views (limit events to range)
#include "qmlprofilerviewmanager.h"

#include <utils/styledbar.h>

#include <QDeclarativeContext>
#include <QToolButton>
#include <QEvent>
#include <QVBoxLayout>
#include <QGraphicsObject>
#include <QScrollBar>
#include <QSlider>
#include <QMenu>

#include <math.h>

using namespace QmlDebug;

namespace QmlProfiler {
namespace Internal {

const int sliderTicks = 10000;
const qreal sliderExp = 3;


/////////////////////////////////////////////////////////
bool MouseWheelResizer::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::Wheel) {
        QWheelEvent *ev = static_cast<QWheelEvent *>(event);
        if (ev->modifiers() & Qt::ControlModifier) {
            emit mouseWheelMoved(ev->pos().x(), ev->pos().y(), ev->delta());
            return true;
        }
    }
    return QObject::eventFilter(obj, event);
}

/////////////////////////////////////////////////////////
void ZoomControl::setRange(qint64 startTime, qint64 endTime)
{
    if (m_startTime != startTime || m_endTime != endTime) {
        m_startTime = startTime;
        m_endTime = endTime;
        emit rangeChanged();
    }
}

/////////////////////////////////////////////////////////
ScrollableDeclarativeView::ScrollableDeclarativeView(QWidget *parent)
    : QDeclarativeView(parent)
{
}

ScrollableDeclarativeView::~ScrollableDeclarativeView()
{
}

void ScrollableDeclarativeView::scrollContentsBy(int dx, int dy)
{
    // special workaround to track the scrollbar
    if (rootObject()) {
        int scrollY = rootObject()->property("scrollY").toInt();
        rootObject()->setProperty("scrollY", QVariant(scrollY - dy));
    }
    QDeclarativeView::scrollContentsBy(dx,dy);
}

/////////////////////////////////////////////////////////
class QmlProfilerTraceView::QmlProfilerTraceViewPrivate
{
public:
    QmlProfilerTraceViewPrivate(QmlProfilerTraceView *qq) : q(qq) {}
    QmlProfilerTraceView *q;

    QmlProfilerStateManager *m_profilerState;
    Analyzer::IAnalyzerTool *m_profilerTool;
    QmlProfilerViewManager *m_viewContainer;

    QSize m_sizeHint;

    ScrollableDeclarativeView *m_mainView;
    QDeclarativeView *m_timebar;
    QDeclarativeView *m_overview;
    QmlProfilerDataModel *m_profilerDataModel;

    ZoomControl *m_zoomControl;

    QToolButton *m_buttonRange;
    QToolButton *m_buttonLock;
    QWidget *m_zoomToolbar;
    int m_currentZoomLevel;
};

QmlProfilerTraceView::QmlProfilerTraceView(QWidget *parent, Analyzer::IAnalyzerTool *profilerTool, QmlProfilerViewManager *container, QmlProfilerDataModel *model, QmlProfilerStateManager *profilerState)
    : QWidget(parent), d(new QmlProfilerTraceViewPrivate(this))
{
    setObjectName(QLatin1String("QML Profiler"));

    d->m_zoomControl = new ZoomControl(this);
    connect(d->m_zoomControl, SIGNAL(rangeChanged()), this, SLOT(updateRange()));

    QVBoxLayout *groupLayout = new QVBoxLayout;
    groupLayout->setContentsMargins(0, 0, 0, 0);
    groupLayout->setSpacing(0);

    d->m_mainView = new ScrollableDeclarativeView(this);
    d->m_mainView->setResizeMode(QDeclarativeView::SizeViewToRootObject);
    d->m_mainView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    d->m_mainView->setBackgroundBrush(QBrush(Qt::white));
    d->m_mainView->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    d->m_mainView->setFocus();

    MouseWheelResizer *resizer = new MouseWheelResizer(this);
    connect(resizer,SIGNAL(mouseWheelMoved(int,int,int)), this, SLOT(mouseWheelMoved(int,int,int)));
    d->m_mainView->viewport()->installEventFilter(resizer);

    QHBoxLayout *toolsLayout = new QHBoxLayout;

    d->m_timebar = new QDeclarativeView(this);
    d->m_timebar->setResizeMode(QDeclarativeView::SizeRootObjectToView);
    d->m_timebar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    d->m_timebar->setFixedHeight(24);

    d->m_overview = new QDeclarativeView(this);
    d->m_overview->setResizeMode(QDeclarativeView::SizeRootObjectToView);
    d->m_overview->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    d->m_overview->setMaximumHeight(50);

    d->m_zoomToolbar = createZoomToolbar();
    d->m_zoomToolbar->move(0, d->m_timebar->height());
    d->m_zoomToolbar->setVisible(false);

    toolsLayout->addWidget(createToolbar());
    toolsLayout->addWidget(d->m_timebar);
    emit enableToolbar(false);

    groupLayout->addLayout(toolsLayout);
    groupLayout->addWidget(d->m_mainView);
    groupLayout->addWidget(d->m_overview);

    setLayout(groupLayout);

    d->m_profilerTool = profilerTool;
    d->m_viewContainer = container;
    d->m_profilerDataModel = model;
    connect(d->m_profilerDataModel, SIGNAL(stateChanged()),
            this, SLOT(profilerDataModelStateChanged()));
    d->m_mainView->rootContext()->setContextProperty(QLatin1String("qmlProfilerDataModel"),
                                                     d->m_profilerDataModel);
    d->m_overview->rootContext()->setContextProperty(QLatin1String("qmlProfilerDataModel"),
                                                     d->m_profilerDataModel);

    d->m_profilerState = profilerState;
    connect(d->m_profilerState, SIGNAL(stateChanged()),
            this, SLOT(profilerStateChanged()));
    connect(d->m_profilerState, SIGNAL(clientRecordingChanged()),
            this, SLOT(clientRecordingChanged()));
    connect(d->m_profilerState, SIGNAL(serverRecordingChanged()),
            this, SLOT(serverRecordingChanged()));

    // Minimum height: 5 rows of 20 pixels + scrollbar of 50 pixels + 20 pixels margin
    setMinimumHeight(170);
    d->m_currentZoomLevel = 0;
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
    d->m_timebar->rootContext()->setContextProperty(QLatin1String("zoomControl"), d->m_zoomControl);
    d->m_overview->rootContext()->setContextProperty(QLatin1String("zoomControl"), d->m_zoomControl);

    d->m_timebar->setSource(QUrl(QLatin1String("qrc:/qmlprofiler/TimeDisplay.qml")));
    d->m_overview->setSource(QUrl(QLatin1String("qrc:/qmlprofiler/Overview.qml")));

    d->m_mainView->setSource(QUrl(QLatin1String("qrc:/qmlprofiler/MainView.qml")));
    QGraphicsObject *rootObject = d->m_mainView->rootObject();
    rootObject->setProperty("width", QVariant(width()));
    rootObject->setProperty("candidateHeight", QVariant(height() - d->m_timebar->height() - d->m_overview->height()));

    connect(rootObject, SIGNAL(updateCursorPosition()), this, SLOT(updateCursorPosition()));
    connect(rootObject, SIGNAL(updateRangeButton()), this, SLOT(updateRangeButton()));
    connect(rootObject, SIGNAL(updateLockButton()), this, SLOT(updateLockButton()));
    connect(this, SIGNAL(jumpToPrev()), rootObject, SLOT(prevEvent()));
    connect(this, SIGNAL(jumpToNext()), rootObject, SLOT(nextEvent()));
    connect(rootObject, SIGNAL(selectedEventChanged(int)), this, SIGNAL(selectedEventChanged(int)));
    connect(rootObject, SIGNAL(changeToolTip(QString)), this, SLOT(updateToolTip(QString)));
    connect(rootObject, SIGNAL(updateVerticalScroll(int)), this, SLOT(updateVerticalScroll(int)));
}

QWidget *QmlProfilerTraceView::createToolbar()
{
    Utils::StyledBar *bar = new Utils::StyledBar(this);
    bar->setStyleSheet(QLatin1String("background: #9B9B9B"));
    bar->setSingleRow(true);
    bar->setFixedWidth(150);
    bar->setFixedHeight(24);

    QHBoxLayout *toolBarLayout = new QHBoxLayout(bar);
    toolBarLayout->setMargin(0);
    toolBarLayout->setSpacing(0);

    QToolButton *buttonPrev= new QToolButton;
    buttonPrev->setIcon(QIcon(QLatin1String(":/qmlprofiler/ico_prev.png")));
    buttonPrev->setToolTip(tr("Jump to previous event"));
    connect(buttonPrev, SIGNAL(clicked()), this, SIGNAL(jumpToPrev()));
    connect(this, SIGNAL(enableToolbar(bool)), buttonPrev, SLOT(setEnabled(bool)));

    QToolButton *buttonNext= new QToolButton;
    buttonNext->setIcon(QIcon(QLatin1String(":/qmlprofiler/ico_next.png")));
    buttonNext->setToolTip(tr("Jump to next event"));
    connect(buttonNext, SIGNAL(clicked()), this, SIGNAL(jumpToNext()));
    connect(this, SIGNAL(enableToolbar(bool)), buttonNext, SLOT(setEnabled(bool)));

    QToolButton *buttonZoomControls = new QToolButton;
    buttonZoomControls->setIcon(QIcon(QLatin1String(":/qmlprofiler/ico_zoom.png")));
    buttonZoomControls->setToolTip(tr("Show zoom slider"));
    buttonZoomControls->setCheckable(true);
    buttonZoomControls->setChecked(false);
    connect(buttonZoomControls, SIGNAL(toggled(bool)), d->m_zoomToolbar, SLOT(setVisible(bool)));
    connect(this, SIGNAL(enableToolbar(bool)), buttonZoomControls, SLOT(setEnabled(bool)));

    d->m_buttonRange = new QToolButton;
    d->m_buttonRange->setIcon(QIcon(QLatin1String(":/qmlprofiler/ico_rangeselection.png")));
    d->m_buttonRange->setToolTip(tr("Select range"));
    d->m_buttonRange->setCheckable(true);
    d->m_buttonRange->setChecked(false);
    connect(d->m_buttonRange, SIGNAL(clicked(bool)), this, SLOT(toggleRangeMode(bool)));
    connect(this, SIGNAL(enableToolbar(bool)), d->m_buttonRange, SLOT(setEnabled(bool)));
    connect(this, SIGNAL(rangeModeChanged(bool)), d->m_buttonRange, SLOT(setChecked(bool)));

    d->m_buttonLock = new QToolButton;
    d->m_buttonLock->setIcon(QIcon(QLatin1String(":/qmlprofiler/ico_selectionmode.png")));
    d->m_buttonLock->setToolTip(tr("View event information on mouseover"));
    d->m_buttonLock->setCheckable(true);
    d->m_buttonLock->setChecked(false);
    connect(d->m_buttonLock, SIGNAL(clicked(bool)), this, SLOT(toggleLockMode(bool)));
    connect(this, SIGNAL(enableToolbar(bool)), d->m_buttonLock, SLOT(setEnabled(bool)));
    connect(this, SIGNAL(lockModeChanged(bool)), d->m_buttonLock, SLOT(setChecked(bool)));

    toolBarLayout->addWidget(buttonPrev);
    toolBarLayout->addWidget(buttonNext);
    toolBarLayout->addWidget(new Utils::StyledSeparator());
    toolBarLayout->addWidget(buttonZoomControls);
    toolBarLayout->addWidget(new Utils::StyledSeparator());
    toolBarLayout->addWidget(d->m_buttonRange);
    toolBarLayout->addWidget(d->m_buttonLock);

    return bar;
}


QWidget *QmlProfilerTraceView::createZoomToolbar()
{
    Utils::StyledBar *bar = new Utils::StyledBar(this);
    bar->setStyleSheet(QLatin1String("background: #9B9B9B"));
    bar->setSingleRow(true);
    bar->setFixedWidth(150);
    bar->setFixedHeight(24);

    QHBoxLayout *toolBarLayout = new QHBoxLayout(bar);
    toolBarLayout->setMargin(0);
    toolBarLayout->setSpacing(0);

    QSlider *zoomSlider = new QSlider(Qt::Horizontal);
    zoomSlider->setFocusPolicy(Qt::NoFocus);
    zoomSlider->setRange(1, sliderTicks);
    zoomSlider->setInvertedAppearance(true);
    zoomSlider->setPageStep(sliderTicks/100);
    connect(this, SIGNAL(enableToolbar(bool)), zoomSlider, SLOT(setEnabled(bool)));
    connect(zoomSlider, SIGNAL(valueChanged(int)), this, SLOT(setZoomLevel(int)));
    connect(this, SIGNAL(zoomLevelChanged(int)), zoomSlider, SLOT(setValue(int)));
    zoomSlider->setStyleSheet(QLatin1String("\
        QSlider:horizontal {\
            background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #444444, stop: 1 #5a5a5a);\
            border: 1px #313131;\
            height: 20px;\
            margin: 0px 0px 0px 0px;\
        }\
        QSlider::add-page:horizontal {\
            background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #5a5a5a, stop: 1 #444444);\
            border: 1px #313131;\
        }\
        QSlider::sub-page:horizontal {\
            background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #5a5a5a, stop: 1 #444444);\
            border: 1px #313131;\
        }\
        "));

    toolBarLayout->addWidget(zoomSlider);

    return bar;
}

/////////////////////////////////////////////////////////
bool QmlProfilerTraceView::hasValidSelection() const
{
    QGraphicsObject *rootObject = d->m_mainView->rootObject();
    if (rootObject)
        return rootObject->property("selectionRangeReady").toBool();
    return false;
}

qint64 QmlProfilerTraceView::selectionStart() const
{
    QGraphicsObject *rootObject = d->m_mainView->rootObject();
    if (rootObject)
        return rootObject->property("selectionRangeStart").toLongLong();
    return 0;
}

qint64 QmlProfilerTraceView::selectionEnd() const
{
    QGraphicsObject *rootObject = d->m_mainView->rootObject();
    if (rootObject)
        return rootObject->property("selectionRangeEnd").toLongLong();
    return 0;
}

void QmlProfilerTraceView::clearDisplay()
{
    d->m_zoomControl->setRange(0,0);

    updateVerticalScroll(0);
    d->m_mainView->rootObject()->setProperty("scrollY", QVariant(0));

    QMetaObject::invokeMethod(d->m_mainView->rootObject(), "clearAll");
    QMetaObject::invokeMethod(d->m_overview->rootObject(), "clearDisplay");
}

void QmlProfilerTraceView::selectNextEventWithId(int eventId)
{
    QGraphicsObject *rootObject = d->m_mainView->rootObject();
    if (rootObject)
        QMetaObject::invokeMethod(rootObject, "selectNextWithId",
                                  Q_ARG(QVariant,QVariant(eventId)));
}

/////////////////////////////////////////////////////////
// Goto source location
void QmlProfilerTraceView::updateCursorPosition()
{
    QGraphicsObject *rootObject = d->m_mainView->rootObject();
    emit gotoSourceLocation(rootObject->property("fileName").toString(),
                            rootObject->property("lineNumber").toInt(),
                            rootObject->property("columnNumber").toInt());
}

/////////////////////////////////////////////////////////
// Toolbar buttons
void QmlProfilerTraceView::toggleRangeMode(bool active)
{
    QGraphicsObject *rootObject = d->m_mainView->rootObject();
    bool rangeMode = rootObject->property("selectionRangeMode").toBool();
    if (active != rangeMode) {
        if (active)
            d->m_buttonRange->setIcon(QIcon(QLatin1String(":/qmlprofiler/ico_rangeselected.png")));
        else
            d->m_buttonRange->setIcon(QIcon(QLatin1String(":/qmlprofiler/ico_rangeselection.png")));
        rootObject->setProperty("selectionRangeMode", QVariant(active));
    }
}

void QmlProfilerTraceView::updateRangeButton()
{
    bool rangeMode = d->m_mainView->rootObject()->property("selectionRangeMode").toBool();
    if (rangeMode)
        d->m_buttonRange->setIcon(QIcon(QLatin1String(":/qmlprofiler/ico_rangeselected.png")));
    else
        d->m_buttonRange->setIcon(QIcon(QLatin1String(":/qmlprofiler/ico_rangeselection.png")));
    emit rangeModeChanged(rangeMode);
}

void QmlProfilerTraceView::toggleLockMode(bool active)
{
    QGraphicsObject *rootObject = d->m_mainView->rootObject();
    bool lockMode = !rootObject->property("selectionLocked").toBool();
    if (active != lockMode) {
        rootObject->setProperty("selectionLocked", QVariant(!active));
        rootObject->setProperty("selectedItem", QVariant(-1));
    }
}

void QmlProfilerTraceView::updateLockButton()
{
    bool lockMode = !d->m_mainView->rootObject()->property("selectionLocked").toBool();
    emit lockModeChanged(lockMode);
}

////////////////////////////////////////////////////////
// Zoom control
void QmlProfilerTraceView::setZoomLevel(int zoomLevel)
{
    if (d->m_currentZoomLevel != zoomLevel && d->m_mainView->rootObject()) {
        QVariant newFactor = pow(qreal(zoomLevel) / qreal(sliderTicks), sliderExp);
        d->m_currentZoomLevel = zoomLevel;
        QMetaObject::invokeMethod(d->m_mainView->rootObject(), "updateWindowLength", Q_ARG(QVariant, newFactor));
    }
}

void QmlProfilerTraceView::updateRange()
{
    if (!d->m_profilerDataModel)
        return;
    qreal duration = d->m_zoomControl->endTime() - d->m_zoomControl->startTime();
    if (duration <= 0)
        return;
    if (d->m_profilerDataModel->traceDuration() <= 0)
        return;
    int newLevel = pow(duration / d->m_profilerDataModel->traceDuration(), 1/sliderExp) * sliderTicks;
    if (d->m_currentZoomLevel != newLevel) {
        d->m_currentZoomLevel = newLevel;
        emit zoomLevelChanged(newLevel);
    }
}

void QmlProfilerTraceView::mouseWheelMoved(int mouseX, int mouseY, int wheelDelta)
{
    Q_UNUSED(mouseY);
    QGraphicsObject *rootObject = d->m_mainView->rootObject();
    if (rootObject) {
        QMetaObject::invokeMethod(rootObject, "wheelZoom",
                                  Q_ARG(QVariant, QVariant(mouseX)),
                                  Q_ARG(QVariant, QVariant(wheelDelta)));
    }
}
////////////////////////////////////////////////////////
void QmlProfilerTraceView::updateToolTip(const QString &text)
{
    setToolTip(text);
}

void QmlProfilerTraceView::updateVerticalScroll(int newPosition)
{
    d->m_mainView->verticalScrollBar()->setValue(newPosition);
}

void QmlProfilerTraceView::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    QGraphicsObject *rootObject = d->m_mainView->rootObject();
    if (rootObject) {
        rootObject->setProperty("width", QVariant(event->size().width()));
        int newHeight = event->size().height() - d->m_timebar->height() - d->m_overview->height();
        rootObject->setProperty("candidateHeight", QVariant(newHeight));
    }
    emit resized();
}

////////////////////////////////////////////////////////////////
// Context menu
void QmlProfilerTraceView::contextMenuEvent(QContextMenuEvent *ev)
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

    QAction *getGlobalStatsAction = menu.addAction(tr("Reset Events Pane"));
    if (d->m_viewContainer->hasGlobalStats())
        getGlobalStatsAction->setEnabled(false);

    if (d->m_profilerDataModel->count() > 0) {
        menu.addSeparator();
        viewAllAction = menu.addAction(tr("Reset Zoom"));
    }

    QAction *selectedAction = menu.exec(ev->globalPos());

    if (selectedAction) {
        if (selectedAction == viewAllAction) {
            d->m_zoomControl->setRange(
                        d->m_profilerDataModel->traceStartTime(),
                        d->m_profilerDataModel->traceEndTime());
        }
        if (selectedAction == getLocalStatsAction) {
            d->m_viewContainer->getStatisticsInRange(
                        d->m_viewContainer->selectionStart(),
                        d->m_viewContainer->selectionEnd());
        }
        if (selectedAction == getGlobalStatsAction) {
            d->m_viewContainer->getStatisticsInRange(
                        d->m_profilerDataModel->traceStartTime(),
                        d->m_profilerDataModel->traceEndTime());
        }
    }
}

/////////////////////////////////////////////////
// Tell QML the state of the profiler
void QmlProfilerTraceView::setRecording(bool recording)
{
    QGraphicsObject *rootObject = d->m_mainView->rootObject();
    if (rootObject)
        rootObject->setProperty("recordingEnabled", QVariant(recording));
}

void QmlProfilerTraceView::setAppKilled()
{
    QGraphicsObject *rootObject = d->m_mainView->rootObject();
    if (rootObject)
        rootObject->setProperty("appKilled",QVariant(true));
}
////////////////////////////////////////////////////////////////
// Profiler State
void QmlProfilerTraceView::profilerDataModelStateChanged()
{
    switch (d->m_profilerDataModel->currentState()) {
    case QmlProfilerDataModel::Empty :
        emit enableToolbar(false);
        break;
    case QmlProfilerDataModel::AcquiringData :
        // nothing to be done
        break;
    case QmlProfilerDataModel::ProcessingData :
        // nothing to be done
        break;
    case QmlProfilerDataModel::Done :
        emit enableToolbar(true);
    break;
    default:
        break;
    }
}

void QmlProfilerTraceView::profilerStateChanged()
{
    switch (d->m_profilerState->currentState()) {
    case QmlProfilerStateManager::AppKilled : {
        if (d->m_profilerDataModel->currentState() == QmlProfilerDataModel::AcquiringData)
            setAppKilled();
        break;
    }
    default:
        // no special action needed for other states
        break;
    }
}

void QmlProfilerTraceView::clientRecordingChanged()
{
    // nothing yet
}

void QmlProfilerTraceView::serverRecordingChanged()
{
    setRecording(d->m_profilerState->serverRecording());
}

} // namespace Internal
} // namespace QmlProfiler
