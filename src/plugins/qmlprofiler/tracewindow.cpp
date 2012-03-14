/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "tracewindow.h"

#include "qmlprofilerplugin.h"

#include <qmljsdebugclient/qmlprofilereventlist.h>
#include <qmljsdebugclient/qmlprofilertraceclient.h>
#include <utils/styledbar.h>

#include <QDeclarativeView>
#include <QDeclarativeContext>
#include <QVBoxLayout>
#include <QGraphicsObject>
#include <QContextMenuEvent>
#include <QScrollBar>
#include <QSlider>
#include <QWidget>

#include <math.h>

using namespace QmlJsDebugClient;

namespace QmlProfiler {
namespace Internal {

const int sliderTicks = 10000;
const qreal sliderExp = 3;

void ZoomControl::setRange(qint64 startTime, qint64 endTime)
{
    if (m_startTime != startTime || m_endTime != endTime) {
        m_startTime = startTime;
        m_endTime = endTime;
        emit rangeChanged();
    }
}

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

TraceWindow::TraceWindow(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("QML Profiler");

    m_zoomControl = new ZoomControl(this);
    connect(m_zoomControl.data(), SIGNAL(rangeChanged()), this, SLOT(updateRange()));

    QVBoxLayout *groupLayout = new QVBoxLayout;
    groupLayout->setContentsMargins(0, 0, 0, 0);
    groupLayout->setSpacing(0);

    m_mainView = new ScrollableDeclarativeView(this);
    m_mainView->setResizeMode(QDeclarativeView::SizeViewToRootObject);
    m_mainView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_mainView->setBackgroundBrush(QBrush(Qt::white));
    m_mainView->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_mainView->setFocus();

    MouseWheelResizer *resizer = new MouseWheelResizer(this);
    connect(resizer,SIGNAL(mouseWheelMoved(int,int,int)), this, SLOT(mouseWheelMoved(int,int,int)));
    m_mainView->viewport()->installEventFilter(resizer);

    QHBoxLayout *toolsLayout = new QHBoxLayout;

    m_timebar = new QDeclarativeView(this);
    m_timebar->setResizeMode(QDeclarativeView::SizeRootObjectToView);
    m_timebar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_timebar->setFixedHeight(24);

    m_overview = new QDeclarativeView(this);
    m_overview->setResizeMode(QDeclarativeView::SizeRootObjectToView);
    m_overview->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_overview->setMaximumHeight(50);

    m_zoomToolbar = createZoomToolbar();
    m_zoomToolbar->move(0, m_timebar->height());
    m_zoomToolbar->setVisible(false);

    toolsLayout->addWidget(createToolbar());
    toolsLayout->addWidget(m_timebar);

    groupLayout->addLayout(toolsLayout);
    groupLayout->addWidget(m_mainView);
    groupLayout->addWidget(m_overview);

    setLayout(groupLayout);

    m_eventList = new QmlProfilerEventList(this);
    connect(this,SIGNAL(range(int,qint64,qint64,QStringList,QmlJsDebugClient::QmlEventLocation)), m_eventList, SLOT(addRangedEvent(int,qint64,qint64,QStringList,QmlJsDebugClient::QmlEventLocation)));
    connect(this, SIGNAL(traceFinished(qint64)), m_eventList, SLOT(setTraceEndTime(qint64)));
    connect(this, SIGNAL(traceStarted(qint64)), m_eventList, SLOT(setTraceStartTime(qint64)));
    connect(this, SIGNAL(frameEvent(qint64,int,int)), m_eventList, SLOT(addFrameEvent(qint64,int,int)));
    connect(m_eventList, SIGNAL(stateChanged()), this, SLOT(eventListStateChanged()));
    m_mainView->rootContext()->setContextProperty("qmlEventList", m_eventList);
    m_overview->rootContext()->setContextProperty("qmlEventList", m_eventList);

    m_rewriter = new QmlProfilerDetailsRewriter(this);
    connect(m_eventList, SIGNAL(requestDetailsForLocation(int,QmlJsDebugClient::QmlEventLocation)), m_rewriter, SLOT(requestDetailsForLocation(int,QmlJsDebugClient::QmlEventLocation)));
    connect(m_rewriter, SIGNAL(rewriteDetailsString(int,QmlJsDebugClient::QmlEventLocation,QString)), m_eventList, SLOT(rewriteDetailsString(int,QmlJsDebugClient::QmlEventLocation,QString)));
    connect(m_rewriter, SIGNAL(eventDetailsChanged()), m_eventList, SLOT(finishedRewritingDetails()));
    connect(m_eventList, SIGNAL(reloadDocumentsForDetails()), m_rewriter, SLOT(reloadDocuments()));

    connect(this, SIGNAL(v8range(int,QString,QString,int,double,double)), m_eventList, SLOT(addV8Event(int,QString,QString,int,double,double)));

    // Minimum height: 5 rows of 20 pixels + scrollbar of 50 pixels + 20 pixels margin
    setMinimumHeight(170);
    m_currentZoomLevel = 0;
    m_profiledTime = 0;

    initializeQmlViews();
}

TraceWindow::~TraceWindow()
{
    disconnectClientSignals();
    delete m_plugin.data();
    delete m_v8plugin.data();
}

QWidget *TraceWindow::createToolbar()
{
    Utils::StyledBar *bar = new Utils::StyledBar(this);
    bar->setSingleRow(true);
    bar->setFixedWidth(150);
    bar->setFixedHeight(24);

    QHBoxLayout *toolBarLayout = new QHBoxLayout(bar);
    toolBarLayout->setMargin(0);
    toolBarLayout->setSpacing(0);

    QToolButton *buttonPrev= new QToolButton;
    buttonPrev->setIcon(QIcon(":/qmlprofiler/ico_prev.png"));
    buttonPrev->setToolTip(tr("Jump to previous event"));
    connect(buttonPrev, SIGNAL(clicked()), this, SIGNAL(jumpToPrev()));
    connect(this, SIGNAL(enableToolbar(bool)), buttonPrev, SLOT(setEnabled(bool)));

    QToolButton *buttonNext= new QToolButton;
    buttonNext->setIcon(QIcon(":/qmlprofiler/ico_next.png"));
    buttonNext->setToolTip(tr("Jump to next event"));
    connect(buttonNext, SIGNAL(clicked()), this, SIGNAL(jumpToNext()));
    connect(this, SIGNAL(enableToolbar(bool)), buttonNext, SLOT(setEnabled(bool)));

    QToolButton *buttonZoomControls = new QToolButton;
    buttonZoomControls->setIcon(QIcon(":/qmlprofiler/ico_zoom.png"));
    buttonZoomControls->setToolTip(tr("Show zoom slider"));
    buttonZoomControls->setCheckable(true);
    buttonZoomControls->setChecked(false);
    connect(buttonZoomControls, SIGNAL(toggled(bool)), m_zoomToolbar, SLOT(setVisible(bool)));
    connect(this, SIGNAL(enableToolbar(bool)), buttonZoomControls, SLOT(setEnabled(bool)));

    m_buttonRange = new QToolButton;
    m_buttonRange->setIcon(QIcon(":/qmlprofiler/ico_rangeselection.png"));
    m_buttonRange->setToolTip(tr("Select range"));
    m_buttonRange->setCheckable(true);
    m_buttonRange->setChecked(false);
    connect(m_buttonRange, SIGNAL(clicked(bool)), this, SLOT(toggleRangeMode(bool)));
    connect(this, SIGNAL(enableToolbar(bool)), m_buttonRange, SLOT(setEnabled(bool)));
    connect(this, SIGNAL(rangeModeChanged(bool)), m_buttonRange, SLOT(setChecked(bool)));

    m_buttonLock = new QToolButton;
    m_buttonLock->setIcon(QIcon(":/qmlprofiler/ico_selectionmode.png"));
    m_buttonLock->setToolTip(tr("View event information on mouseover"));
    m_buttonLock->setCheckable(true);
    m_buttonLock->setChecked(false);
    connect(m_buttonLock, SIGNAL(clicked(bool)), this, SLOT(toggleLockMode(bool)));
    connect(this, SIGNAL(enableToolbar(bool)), m_buttonLock, SLOT(setEnabled(bool)));
    connect(this, SIGNAL(lockModeChanged(bool)), m_buttonLock, SLOT(setChecked(bool)));

    toolBarLayout->addWidget(buttonPrev);
    toolBarLayout->addWidget(buttonNext);
    toolBarLayout->addWidget(new Utils::StyledSeparator());
    toolBarLayout->addWidget(buttonZoomControls);
    toolBarLayout->addWidget(new Utils::StyledSeparator());
    toolBarLayout->addWidget(m_buttonRange);
    toolBarLayout->addWidget(m_buttonLock);

    return bar;
}


QWidget *TraceWindow::createZoomToolbar()
{
    Utils::StyledBar *bar = new Utils::StyledBar(this);
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
    zoomSlider->setStyleSheet("\
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
        ");

    toolBarLayout->addWidget(zoomSlider);

    return bar;
}

void TraceWindow::reset(QDeclarativeDebugConnection *conn)
{
    disconnectClientSignals();

    delete m_plugin.data();
    m_plugin = new QmlProfilerTraceClient(conn);
    delete m_v8plugin.data();
    m_v8plugin = new QV8ProfilerClient(conn);

    connectClientSignals();

    m_v8DataReady = false;
    m_qmlDataReady = false;

    m_mainView->rootContext()->setContextProperty("connection", m_plugin.data());
}

void TraceWindow::initializeQmlViews()
{
    m_mainView->rootContext()->setContextProperty("connection", m_plugin.data());
    m_mainView->rootContext()->setContextProperty("zoomControl", m_zoomControl.data());
    m_timebar->rootContext()->setContextProperty("zoomControl", m_zoomControl.data());
    m_overview->rootContext()->setContextProperty("zoomControl", m_zoomControl.data());

    m_timebar->setSource(QUrl("qrc:/qmlprofiler/TimeDisplay.qml"));
    m_overview->setSource(QUrl("qrc:/qmlprofiler/Overview.qml"));

    m_mainView->setSource(QUrl("qrc:/qmlprofiler/MainView.qml"));
    m_mainView->rootObject()->setProperty("width", QVariant(width()));
    m_mainView->rootObject()->setProperty("candidateHeight", QVariant(height() - m_timebar->height() - m_overview->height()));

    updateToolbar();

    connect(m_mainView->rootObject(), SIGNAL(updateCursorPosition()), this, SLOT(updateCursorPosition()));
    connect(m_mainView->rootObject(), SIGNAL(updateTimer()), this, SLOT(updateTimer()));
    connect(m_mainView->rootObject(), SIGNAL(updateRangeButton()), this, SLOT(updateRangeButton()));
    connect(m_mainView->rootObject(), SIGNAL(updateLockButton()), this, SLOT(updateLockButton()));
    connect(m_eventList, SIGNAL(countChanged()), this, SLOT(updateToolbar()));
    connect(this, SIGNAL(jumpToPrev()), m_mainView->rootObject(), SLOT(prevEvent()));
    connect(this, SIGNAL(jumpToNext()), m_mainView->rootObject(), SLOT(nextEvent()));
    connect(this, SIGNAL(updateViewZoom(QVariant)), m_mainView->rootObject(), SLOT(updateWindowLength(QVariant)));
    connect(this, SIGNAL(wheelZoom(QVariant,QVariant)), m_mainView->rootObject(), SLOT(wheelZoom(QVariant,QVariant)));
    connect(this, SIGNAL(globalZoom()), m_mainView->rootObject(), SLOT(globalZoom()));
    connect(this, SIGNAL(selectNextEventInDisplay(QVariant)), m_mainView->rootObject(), SLOT(selectNextWithId(QVariant)));
    connect(m_mainView->rootObject(), SIGNAL(selectedEventIdChanged(int)), this, SIGNAL(selectedEventIdChanged(int)));
    connect(m_mainView->rootObject(), SIGNAL(changeToolTip(QString)), this, SLOT(updateToolTip(QString)));
    connect(m_mainView->rootObject(), SIGNAL(updateVerticalScroll(int)), this, SLOT(updateVerticalScroll(int)));

    connect(this, SIGNAL(internalClearDisplay()), m_mainView->rootObject(), SLOT(clearAll()));
    connect(this,SIGNAL(internalClearDisplay()), m_overview->rootObject(), SLOT(clearDisplay()));
}

void TraceWindow::connectClientSignals()
{
    if (m_plugin) {
        connect(m_plugin.data(), SIGNAL(complete()), this, SLOT(qmlComplete()));
        connect(m_plugin.data(), SIGNAL(range(int,qint64,qint64,QStringList,QmlJsDebugClient::QmlEventLocation)),
                this, SIGNAL(range(int,qint64,qint64,QStringList,QmlJsDebugClient::QmlEventLocation)));
        connect(m_plugin.data(), SIGNAL(traceFinished(qint64)), this, SIGNAL(traceFinished(qint64)));
        connect(m_plugin.data(), SIGNAL(traceStarted(qint64)), this, SLOT(manageTraceStart(qint64)));
        connect(m_plugin.data(), SIGNAL(frame(qint64,int,int)), this, SIGNAL(frameEvent(qint64,int,int)));
        connect(m_plugin.data(), SIGNAL(enabledChanged()), this, SLOT(updateProfilerState()));
        connect(m_plugin.data(), SIGNAL(enabledChanged()), m_plugin.data(), SLOT(sendRecordingStatus()));
        connect(m_plugin.data(), SIGNAL(recordingChanged(bool)), this, SIGNAL(recordingChanged(bool)));
    }
    if (m_v8plugin) {
        connect(m_v8plugin.data(), SIGNAL(complete()), this, SLOT(v8Complete()));
        connect(m_v8plugin.data(), SIGNAL(v8range(int,QString,QString,int,double,double)), this, SIGNAL(v8range(int,QString,QString,int,double,double)));
        connect(m_v8plugin.data(), SIGNAL(enabledChanged()), this, SLOT(updateProfilerState()));
        connect(m_v8plugin.data(), SIGNAL(enabledChanged()), m_v8plugin.data(), SLOT(sendRecordingStatus()));
    }
}

void TraceWindow::disconnectClientSignals()
{
    if (m_plugin) {
        disconnect(m_plugin.data(), SIGNAL(complete()), this, SLOT(qmlComplete()));
        disconnect(m_plugin.data(), SIGNAL(range(int,qint64,qint64,QStringList,QmlJsDebugClient::QmlEventLocation)),
                this, SIGNAL(range(int,qint64,qint64,QStringList,QmlJsDebugClient::QmlEventLocation)));
        disconnect(m_plugin.data(), SIGNAL(traceFinished(qint64)), this, SIGNAL(traceFinished(qint64)));
        disconnect(m_plugin.data(), SIGNAL(traceStarted(qint64)), this, SLOT(manageTraceStart(qint64)));
        disconnect(m_plugin.data(), SIGNAL(enabledChanged()), this, SLOT(updateProfilerState()));
        disconnect(m_plugin.data(), SIGNAL(enabledChanged()), m_plugin.data(), SLOT(sendRecordingStatus()));
        disconnect(m_plugin.data(), SIGNAL(recordingChanged(bool)), this, SIGNAL(recordingChanged(bool)));
    }
    if (m_v8plugin) {
        disconnect(m_v8plugin.data(), SIGNAL(complete()), this, SLOT(v8Complete()));
        disconnect(m_v8plugin.data(), SIGNAL(v8range(int,QString,QString,int,double,double)), this, SIGNAL(v8range(int,QString,QString,int,double,double)));
        disconnect(m_v8plugin.data(), SIGNAL(enabledChanged()), this, SLOT(updateProfilerState()));
        disconnect(m_v8plugin.data(), SIGNAL(enabledChanged()), m_v8plugin.data(), SLOT(sendRecordingStatus()));
    }
}

QmlProfilerEventList *TraceWindow::getEventList() const
{
    return m_eventList;
}

ZoomControl *TraceWindow::rangeTimes() const
{
    return m_zoomControl.data();
}

void TraceWindow::contextMenuEvent(QContextMenuEvent *ev)
{
    emit contextMenuRequested(ev->globalPos());
}

void TraceWindow::updateCursorPosition()
{
    emit gotoSourceLocation(m_mainView->rootObject()->property("fileName").toString(),
                            m_mainView->rootObject()->property("lineNumber").toInt(),
                            m_mainView->rootObject()->property("columnNumber").toInt());
}

void TraceWindow::updateTimer()
{
    m_profiledTime = m_mainView->rootObject()->property("elapsedTime").toDouble();
}

void TraceWindow::correctTimer()
{
    // once the data is post-processed, use the eventlist time instead of the qml timer
    m_profiledTime = (m_eventList->traceEndTime() - m_eventList->traceStartTime()) / 1.0e9;
    if (m_profiledTime < 0)
        m_profiledTime = 0;
    emit viewUpdated();
}

double TraceWindow::profiledTime() const
{
    return m_profiledTime;
}

void TraceWindow::clearDisplay()
{
    m_eventList->clear();

    if (m_plugin)
        m_plugin.data()->clearData();
    if (m_v8plugin)
        m_v8plugin.data()->clearData();

    m_zoomControl.data()->setRange(0,0);
    m_profiledTime = 0;

    updateVerticalScroll(0);
    m_mainView->rootObject()->setProperty("scrollY", QVariant(0));

    emit internalClearDisplay();
}

void TraceWindow::updateToolbar()
{
    emit enableToolbar(m_eventList && m_eventList->count()>0);
}

void TraceWindow::toggleRangeMode(bool active)
{
    bool rangeMode = m_mainView->rootObject()->property("selectionRangeMode").toBool();
    if (active != rangeMode) {
        if (active)
            m_buttonRange->setIcon(QIcon(":/qmlprofiler/ico_rangeselected.png"));
        else
            m_buttonRange->setIcon(QIcon(":/qmlprofiler/ico_rangeselection.png"));
        m_mainView->rootObject()->setProperty("selectionRangeMode", QVariant(active));
    }
}

void TraceWindow::updateRangeButton()
{
    bool rangeMode = m_mainView->rootObject()->property("selectionRangeMode").toBool();
    if (rangeMode)
        m_buttonRange->setIcon(QIcon(":/qmlprofiler/ico_rangeselected.png"));
    else
        m_buttonRange->setIcon(QIcon(":/qmlprofiler/ico_rangeselection.png"));
    emit rangeModeChanged(rangeMode);
}

void TraceWindow::toggleLockMode(bool active)
{
    bool lockMode = !m_mainView->rootObject()->property("selectionLocked").toBool();
    if (active != lockMode) {
        m_mainView->rootObject()->setProperty("selectionLocked", QVariant(!active));
        m_mainView->rootObject()->setProperty("selectedItem", QVariant(-1));
    }
}

void TraceWindow::updateLockButton()
{
    bool lockMode = !m_mainView->rootObject()->property("selectionLocked").toBool();
    emit lockModeChanged(lockMode);
}

void TraceWindow::setRecording(bool recording)
{
    if (recording) {
        m_v8DataReady = false;
        m_qmlDataReady = false;
    }
    if (m_plugin)
        m_plugin.data()->setRecording(recording);
    if (m_v8plugin)
        m_v8plugin.data()->setRecording(recording);
}

bool TraceWindow::isRecording() const
{
    return m_plugin.data()->isRecording();
}

void TraceWindow::qmlComplete()
{
    m_qmlDataReady = true;
    if (!m_v8plugin || m_v8plugin.data()->status() != QDeclarativeDebugClient::Enabled || m_v8DataReady) {
        m_eventList->complete();

        // if no data was received, still notify completion
        if (m_eventList->currentState() == QmlProfilerEventList::Empty)
            emit viewUpdated();

        // once complete is sent, reset the flags
        m_qmlDataReady = false;
        m_v8DataReady = false;
    }
}

void TraceWindow::v8Complete()
{
    m_v8DataReady = true;
    if (!m_plugin || m_plugin.data()->status() != QDeclarativeDebugClient::Enabled || m_qmlDataReady) {
        m_eventList->complete();

        // if no data was received, still notify completion
        if (m_eventList->currentState() == QmlProfilerEventList::Empty)
            emit viewUpdated();

        // once complete is sent, reset the flags
        m_v8DataReady = false;
        m_qmlDataReady = false;
    }
}

void TraceWindow::resizeEvent(QResizeEvent *event)
{
    if (m_mainView->rootObject()) {
        m_mainView->rootObject()->setProperty("width", QVariant(event->size().width()));
        int newHeight = event->size().height() - m_timebar->height() - m_overview->height();
        m_mainView->rootObject()->setProperty("candidateHeight", QVariant(newHeight));
    }
}

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

void TraceWindow::mouseWheelMoved(int x, int y, int delta)
{
    Q_UNUSED(y);
    if (m_mainView->rootObject()) {
        emit wheelZoom(QVariant(x), QVariant(delta));
    }
}

void TraceWindow::viewAll()
{
    emit globalZoom();
}

bool TraceWindow::hasValidSelection() const
{
    if (m_mainView->rootObject()) {
        return m_mainView->rootObject()->property("selectionRangeReady").toBool();
    }
    return false;
}

qint64 TraceWindow::selectionStart() const
{
    if (m_mainView->rootObject()) {
        return m_mainView->rootObject()->property("selectionRangeStart").toLongLong();
    }
    return 0;
}

qint64 TraceWindow::selectionEnd() const
{
    if (m_mainView->rootObject()) {
        return m_mainView->rootObject()->property("selectionRangeEnd").toLongLong();
    }
    return 0;
}

void TraceWindow::setZoomLevel(int zoomLevel)
{
    if (m_currentZoomLevel != zoomLevel && m_mainView->rootObject()) {
        qreal newFactor = pow(qreal(zoomLevel) / qreal(sliderTicks), sliderExp);
        m_currentZoomLevel = zoomLevel;
        emit updateViewZoom(QVariant(newFactor));
    }
}

void TraceWindow::updateRange()
{
    if (!m_eventList)
        return;
    qreal duration = m_zoomControl.data()->endTime() - m_zoomControl.data()->startTime();
    if (duration <= 0)
        return;
    if (m_eventList->traceDuration() <= 0)
        return;
    int newLevel = pow(duration / m_eventList->traceDuration(), 1/sliderExp) * sliderTicks;
    if (m_currentZoomLevel != newLevel) {
        m_currentZoomLevel = newLevel;
        emit zoomLevelChanged(newLevel);
    }
}

void TraceWindow::selectNextEvent(int eventId)
{
    emit selectNextEventInDisplay(QVariant(eventId));
}

void TraceWindow::updateProfilerState()
{
    bool qmlActive = false;
    bool v8Active = false;
    if (m_plugin)
        qmlActive = m_plugin.data()->isEnabled();
    if (m_v8plugin)
        v8Active = m_v8plugin.data()->isEnabled();

    emit profilerStateChanged(qmlActive, v8Active);
}

void TraceWindow::updateToolTip(const QString &text)
{
    setToolTip(text);
}

void TraceWindow::updateVerticalScroll(int newPosition)
{
    m_mainView->verticalScrollBar()->setValue(newPosition);
}

void TraceWindow::eventListStateChanged()
{
    switch (m_eventList->currentState()) {
    case QmlProfilerEventList::Empty :
        clearDisplay();
        break;
    case QmlProfilerEventList::AcquiringData :
        firstDataReceived();
        break;
    case QmlProfilerEventList::ProcessingData :
        // nothing to be done
        break;
    case QmlProfilerEventList::Done :
        correctTimer();
    break;
    default:
        break;
    }
}

void TraceWindow::manageTraceStart(qint64 traceStart)
{
    // new trace started
    emit clearViewsFromTool();

    emit traceStarted(traceStart);
}

void TraceWindow::firstDataReceived()
{
    if (m_plugin && m_plugin.data()->isRecording()) {
        // serverside recording disabled
        m_plugin.data()->setRecordingFromServer(false);
    }
}

void TraceWindow::applicationDied()
{
    if (m_mainView->rootObject())
        m_mainView->rootObject()->setProperty("applicationDied",QVariant(true));
}

} // namespace Internal
} // namespace QmlProfiler
