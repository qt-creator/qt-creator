/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "tracewindow.h"

#include "qmlprofilerplugin.h"

#include <qmljsdebugclient/qmlprofilereventlist.h>
#include <qmljsdebugclient/qmlprofilertraceclient.h>
#include <utils/styledbar.h>

#include <QtDeclarative/QDeclarativeView>
#include <QtDeclarative/QDeclarativeContext>
#include <QtGui/QVBoxLayout>
#include <QtGui/QToolButton>
#include <QtGui/QGraphicsObject>
#include <QtGui/QContextMenuEvent>

using namespace QmlJsDebugClient;

namespace QmlProfiler {
namespace Internal {

TraceWindow::TraceWindow(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("QML Profiler");

    QVBoxLayout *groupLayout = new QVBoxLayout;
    groupLayout->setContentsMargins(0, 0, 0, 0);
    groupLayout->setSpacing(0);

    m_view = new QDeclarativeView(this);

    if (QmlProfilerPlugin::debugOutput) {
        //new QmlJSDebugger::JSDebuggerAgent(m_view->engine());
        //new QmlJSDebugger::QDeclarativeViewObserver(m_view, m_view);
    }

    Utils::StyledBar *bar = new Utils::StyledBar(this);
    bar->setSingleRow(true);
    bar->setMinimumWidth(150);
    QHBoxLayout *toolBarLayout = new QHBoxLayout(bar);
    toolBarLayout->setMargin(0);
    toolBarLayout->setSpacing(0);
    QToolButton *buttonPrev= new QToolButton;
    buttonPrev->setIcon(QIcon(":/qmlprofiler/prev.png"));
    buttonPrev->setToolTip(tr("Jump to previous event"));
    connect(buttonPrev, SIGNAL(clicked()), this, SIGNAL(jumpToPrev()));
    connect(this, SIGNAL(enableToolbar(bool)), buttonPrev, SLOT(setEnabled(bool)));
    QToolButton *buttonNext= new QToolButton;
    buttonNext->setIcon(QIcon(":/qmlprofiler/next.png"));
    buttonNext->setToolTip(tr("Jump to next event"));
    connect(buttonNext, SIGNAL(clicked()), this, SIGNAL(jumpToNext()));
    connect(this, SIGNAL(enableToolbar(bool)), buttonNext, SLOT(setEnabled(bool)));
    QToolButton *buttonZoomIn = new QToolButton;
    buttonZoomIn->setIcon(QIcon(":/qmlprofiler/magnifier-plus.png"));
    buttonZoomIn->setToolTip(tr("Zoom in 10%"));
    connect(buttonZoomIn, SIGNAL(clicked()), this, SIGNAL(zoomIn()));
    connect(this, SIGNAL(enableToolbar(bool)), buttonZoomIn, SLOT(setEnabled(bool)));
    QToolButton *buttonZoomOut = new QToolButton;
    buttonZoomOut->setIcon(QIcon(":/qmlprofiler/magnifier-minus.png"));
    buttonZoomOut->setToolTip(tr("Zoom out 10%"));
    connect(buttonZoomOut, SIGNAL(clicked()), this, SIGNAL(zoomOut()));
    connect(this, SIGNAL(enableToolbar(bool)), buttonZoomOut, SLOT(setEnabled(bool)));

    toolBarLayout->addWidget(buttonPrev);
    toolBarLayout->addWidget(buttonNext);
    toolBarLayout->addWidget(buttonZoomIn);
    toolBarLayout->addWidget(buttonZoomOut);

    m_view->setResizeMode(QDeclarativeView::SizeRootObjectToView);
    m_view->setFocus();
    groupLayout->addWidget(m_view);

    setLayout(groupLayout);

    m_eventList = new QmlProfilerEventList(this);
    connect(this,SIGNAL(range(int,qint64,qint64,QStringList,QString,int)), m_eventList, SLOT(addRangedEvent(int,qint64,qint64,QStringList,QString,int)));
    connect(this, SIGNAL(traceFinished(qint64)), m_eventList, SLOT(setTraceEndTime(qint64)));
    connect(this,SIGNAL(viewUpdated()), m_eventList, SLOT(complete()));
    m_view->rootContext()->setContextProperty("qmlEventList", m_eventList);

    connect(this, SIGNAL(v8range(int,QString,QString,int,double,double)), m_eventList, SLOT(addV8Event(int,QString,QString,int,double,double)));

    // Minimum height: 5 rows of 20 pixels + scrollbar of 50 pixels + 20 pixels margin
    setMinimumHeight(170);
}

TraceWindow::~TraceWindow()
{
    delete m_plugin.data();
    delete m_v8plugin.data();
}

void TraceWindow::reset(QDeclarativeDebugConnection *conn)
{
    if (m_plugin)
        disconnect(m_plugin.data(), SIGNAL(complete()), this, SLOT(qmlComplete()));
    delete m_plugin.data();
    m_plugin = new QmlProfilerTraceClient(conn);
    connect(m_plugin.data(), SIGNAL(complete()), this, SLOT(qmlComplete()));
    connect(m_plugin.data(), SIGNAL(range(int,qint64,qint64,QStringList,QString,int)),
            this, SIGNAL(range(int,qint64,qint64,QStringList,QString,int)));

    if (m_v8plugin) {
        disconnect(m_v8plugin.data(), SIGNAL(complete()), this, SLOT(v8Complete()));
        disconnect(m_v8plugin.data(), SIGNAL(v8range(int,QString,QString,int,double,double)), this, SIGNAL(v8range(int,QString,QString,int,double,double)));
    }
    delete m_v8plugin.data();
    m_v8plugin = new QV8ProfilerClient(conn);
    connect(m_v8plugin.data(), SIGNAL(complete()), this, SLOT(v8Complete()));
    connect(m_v8plugin.data(), SIGNAL(v8range(int,QString,QString,int,double,double)), this, SIGNAL(v8range(int,QString,QString,int,double,double)));
    connect(m_plugin.data(), SIGNAL(traceFinished(qint64)), this, SIGNAL(traceFinished(qint64)));

    m_view->rootContext()->setContextProperty("connection", m_plugin.data());
    m_view->setSource(QUrl("qrc:/qmlprofiler/MainView.qml"));

    updateToolbar();

    connect(m_view->rootObject(), SIGNAL(updateCursorPosition()), this, SLOT(updateCursorPosition()));
    connect(m_view->rootObject(), SIGNAL(updateTimer()), this, SLOT(updateTimer()));
    connect(m_eventList, SIGNAL(countChanged()), this, SLOT(updateToolbar()));
    connect(this, SIGNAL(jumpToPrev()), m_view->rootObject(), SLOT(prevEvent()));
    connect(this, SIGNAL(jumpToNext()), m_view->rootObject(), SLOT(nextEvent()));
    connect(this, SIGNAL(zoomIn()), m_view->rootObject(), SLOT(zoomIn()));
    connect(this, SIGNAL(zoomOut()), m_view->rootObject(), SLOT(zoomOut()));

    connect(this, SIGNAL(internalClearDisplay()), m_view->rootObject(), SLOT(clearAll()));

    m_v8DataReady = false;
    m_qmlDataReady = false;
}

QmlProfilerEventList *TraceWindow::getEventList() const
{
    return m_eventList;
}

void TraceWindow::contextMenuEvent(QContextMenuEvent *ev)
{
    emit contextMenuRequested(ev->globalPos());
}

void TraceWindow::updateCursorPosition()
{
    emit gotoSourceLocation(m_view->rootObject()->property("fileName").toString(),
                            m_view->rootObject()->property("lineNumber").toInt());
}

void TraceWindow::updateTimer()
{
    emit timeChanged(m_view->rootObject()->property("elapsedTime").toDouble());
}

void TraceWindow::clearDisplay()
{
    m_eventList->clear();

    if (m_plugin)
        m_plugin.data()->clearData();
    if (m_v8plugin)
        m_v8plugin.data()->clearData();

    emit internalClearDisplay();
}

void TraceWindow::updateToolbar()
{
    emit enableToolbar(m_eventList && m_eventList->count()>0);
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

    if (!m_v8plugin || m_v8plugin.data()->status() != QDeclarativeDebugClient::Enabled || m_v8DataReady)
        emit viewUpdated();
}

void TraceWindow::v8Complete()
{
    m_v8DataReady = true;
    if (!m_plugin || m_plugin.data()->status() != QDeclarativeDebugClient::Enabled || m_qmlDataReady)
        emit viewUpdated();
}

} // namespace Internal
} // namespace QmlProfiler
