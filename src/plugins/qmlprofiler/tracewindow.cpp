/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "tracewindow.h"

#include "qmlprofilerplugin.h"
#include "qmlprofilertraceclient.h"

#include <qmljsdebugclient/qdeclarativedebugclient_p.h>
#include <utils/styledbar.h>

#include <QtDeclarative/QDeclarativeView>
#include <QtDeclarative/QDeclarativeContext>
#include <QtGui/QVBoxLayout>
#include <QtGui/QToolButton>
#include <QtGui/QGraphicsObject>

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
    connect(buttonPrev, SIGNAL(clicked()), this, SIGNAL(jumpToPrev()));
    QToolButton *buttonNext= new QToolButton;
    buttonNext->setIcon(QIcon(":/qmlprofiler/next.png"));
    connect(buttonNext, SIGNAL(clicked()), this, SIGNAL(jumpToNext()));
    QToolButton *buttonZoomIn = new QToolButton;
    buttonZoomIn->setIcon(QIcon(":/qmlprofiler/magnifier-plus.png"));
    connect(buttonZoomIn, SIGNAL(clicked()), this, SIGNAL(zoomIn()));
    QToolButton *buttonZoomOut = new QToolButton;
    buttonZoomOut->setIcon(QIcon(":/qmlprofiler/magnifier-minus.png"));
    connect(buttonZoomOut, SIGNAL(clicked()), this, SIGNAL(zoomOut()));
    toolBarLayout->addWidget(buttonPrev);
    toolBarLayout->addWidget(buttonNext);
    toolBarLayout->addWidget(buttonZoomIn);
    toolBarLayout->addWidget(buttonZoomOut);

    m_view->setResizeMode(QDeclarativeView::SizeRootObjectToView);
    m_view->setFocus();
    groupLayout->addWidget(m_view);

    setLayout(groupLayout);

    // Minimum height: 5 rows of 20 pixels + scrollbar of 50 pixels + 20 pixels margin
    setMinimumHeight(170);
}

TraceWindow::~TraceWindow()
{
    delete m_plugin.data();
}

void TraceWindow::reset(QmlJsDebugClient::QDeclarativeDebugConnection *conn)
{
    if (m_plugin)
        disconnect(m_plugin.data(), SIGNAL(complete()), this, SIGNAL(viewUpdated()));
    delete m_plugin.data();
    m_plugin = new QmlProfilerTraceClient(conn);
    connect(m_plugin.data(), SIGNAL(complete()), this, SIGNAL(viewUpdated()));
    connect(m_plugin.data(), SIGNAL(range(int,int,int,qint64,qint64,QStringList,QString,int)),
            this, SIGNAL(range(int,int,int,qint64,qint64,QStringList,QString,int)));

    m_view->rootContext()->setContextProperty("connection", m_plugin.data());
    m_view->setSource(QUrl("qrc:/qmlprofiler/MainView.qml"));

    connect(m_view->rootObject(), SIGNAL(updateCursorPosition()), this, SLOT(updateCursorPosition()));
    connect(m_view->rootObject(), SIGNAL(updateTimer()), this, SLOT(updateTimer()));
    connect(this, SIGNAL(jumpToPrev()), m_view->rootObject(), SLOT(prevEvent()));
    connect(this, SIGNAL(jumpToNext()), m_view->rootObject(), SLOT(nextEvent()));
    connect(this, SIGNAL(zoomIn()), m_view->rootObject(), SLOT(zoomIn()));
    connect(this, SIGNAL(zoomOut()), m_view->rootObject(), SLOT(zoomOut()));

    connect(this, SIGNAL(internalClearDisplay()), m_view->rootObject(), SLOT(clearAll()));
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
    if (m_plugin)
        m_plugin.data()->clearView();
    else
        emit internalClearDisplay();
}

void TraceWindow::setRecording(bool recording)
{
    if (m_plugin)
        m_plugin.data()->setRecording(recording);
}

bool TraceWindow::isRecording() const
{
    return m_plugin.data()->isRecording();
}

} // namespace Internal
} // namespace QmlProfiler
