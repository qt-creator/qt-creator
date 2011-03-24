/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/
#include "tracewindow.h"

#include <QtCore/qdebug.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qdatastream.h>
#include <QtCore/qmargins.h>

#include <QtGui/qapplication.h>
#include <QtGui/qpainter.h>
#include <QtGui/qboxlayout.h>
#include <QtGui/qevent.h>
#include <QtCore/qstack.h>

#include <QtOpenGL/QGLWidget>
#include <QGraphicsObject>

#include <QtDeclarative/qdeclarativeview.h>
#include <QtDeclarative/qdeclarativecontext.h>
#include <QtDeclarative/qdeclarative.h>

#include "qmlprofilerplugin.h"
//#include <jsdebuggeragent.h>
//#include <qdeclarativeviewobserver.h>

#define GAP_TIME 150

struct Location
{
    Location() : line(-1) {}
    Location(const QString &file, int lineNumber) : fileName(file), line(lineNumber) {}
    QString fileName;
    int line;
};

class TracePlugin : public QDeclarativeDebugClient
{
    Q_OBJECT
    Q_PROPERTY(bool recording READ recording WRITE setRecording NOTIFY recordingChanged)
public:
    TracePlugin(QDeclarativeDebugConnection *client);

    enum EventType {
        FramePaint,
        Mouse,
        Key,

        MaximumEventType
    };

    enum Message {
        Event,
        RangeStart,
        RangeData,
        RangeLocation,
        RangeEnd,
        Complete,

        MaximumMessage
    };

    enum RangeType {
        Painting,
        Compiling,
        Creating,
        Binding,
        HandlingSignal,

        MaximumRangeType
    };

    bool recording() const
    {
        return m_recording;
    }

public slots:
    void setRecording(bool);

signals:
    void complete();
    void gap(qint64);
    void event(int event, qint64 time);
    void range(int type, qint64 startTime, qint64 length, const QStringList &data, const QString &fileName, int line);

    void sample(int, int, int, bool);

    void recordingChanged(bool arg);

    void enabled();

protected:
    virtual void statusChanged(Status);
    virtual void messageReceived(const QByteArray &);

private:

    qint64 m_inProgressRanges;
    QStack<qint64> m_rangeStartTimes[MaximumRangeType];
    QStack<QStringList> m_rangeDatas[MaximumRangeType];
    QStack<Location> m_rangeLocations[MaximumRangeType];
    int m_rangeCount[MaximumRangeType];
    qint64 m_maximumTime;
    bool m_recording;
};

TracePlugin::TracePlugin(QDeclarativeDebugConnection *client)
    : QDeclarativeDebugClient(QLatin1String("CanvasFrameRate"), client), m_inProgressRanges(0), m_maximumTime(0), m_recording(false)
{
    ::memset(m_rangeCount, 0, MaximumRangeType * sizeof(int));
}

void TracePlugin::setRecording(bool v)
{
    if (v == m_recording)
        return;
    QByteArray ba;
    QDataStream stream(&ba, QIODevice::WriteOnly);
    stream << v;
    sendMessage(ba);
    m_recording = v;
    emit recordingChanged(v);
}

void TracePlugin::statusChanged(Status status)
{
    if (status == Enabled) {
        m_recording = !m_recording;
        setRecording(!m_recording);
        emit enabled();
    }
}

void TracePlugin::messageReceived(const QByteArray &data)
{
    QByteArray rwData = data;
    QDataStream stream(&rwData, QIODevice::ReadOnly);

    qint64 time;
    int messageType;

    stream >> time >> messageType;

//    qDebug() << __FUNCTION__ << messageType;

    if (messageType >= MaximumMessage)
        return;

    if (time > (m_maximumTime + GAP_TIME) && 0 == m_inProgressRanges)
        emit gap(time);

    if (messageType == Event) {
        int event;
        stream >> event;

        if (event < MaximumEventType) {
            emit this->event((EventType)event, time);
            m_maximumTime = qMax(time, m_maximumTime);
        }
    } else if (messageType == Complete) {
        emit complete();
    } else {
        int range;
        stream >> range;

        if (range >= MaximumRangeType)
            return;

        if (messageType == RangeStart) {
            m_rangeStartTimes[range].push(time);
            m_inProgressRanges |= (1 << range);
            ++m_rangeCount[range];
        } else if (messageType == RangeData) {
            QString data;
            stream >> data;

            int count = m_rangeCount[range];
            if (count > 0) {
                while (m_rangeDatas[range].count() < count)
                    m_rangeDatas[range].push(QStringList());
                m_rangeDatas[range][count-1] << data;
            }

        } else if (messageType == RangeLocation) {
            QString fileName;
            int line;
            stream >> fileName >> line;

            if (m_rangeCount[range] > 0) {
                m_rangeLocations[range].push(Location(fileName, line));
            }
        } else {
            if (m_rangeCount[range] > 0) {
                --m_rangeCount[range];
                if (m_inProgressRanges & (1 << range))
                    m_inProgressRanges &= ~(1 << range);

                m_maximumTime = qMax(time, m_maximumTime);
                QStringList data = m_rangeDatas[range].count() ? m_rangeDatas[range].pop() : QStringList();
                Location location = m_rangeLocations[range].count() ? m_rangeLocations[range].pop() : Location();

                qint64 startTime = m_rangeStartTimes[range].pop();
                emit this->range((RangeType)range, startTime, time - startTime, data, location.fileName, location.line);
                if (m_rangeCount[range] == 0) {
                    int count = m_rangeDatas[range].count() +
                                m_rangeStartTimes[range].count() +
                                m_rangeLocations[range].count();
                    if (count != 0)
                        qWarning() << "incorrectly nested data";
                }
            }
        }
    }
}

TraceWindow::TraceWindow(QWidget *parent)
: QWidget(parent),
  m_plugin(0), m_recordAtStart(false)
{
    setObjectName(tr("Qml Perfomance Monitor"));

    QVBoxLayout *groupLayout = new QVBoxLayout;
    groupLayout->setContentsMargins(0, 0, 0, 0);
    groupLayout->setSpacing(0);

    m_view = new QDeclarativeView(this);

    if (Analyzer::Internal::QmlProfilerPlugin::debugOutput) {
        //new QmlJSDebugger::JSDebuggerAgent(m_view->engine());
        //new QmlJSDebugger::QDeclarativeViewObserver(m_view, m_view);
    }

    m_view->setViewport(new QGLWidget());
    m_view->setResizeMode(QDeclarativeView::SizeRootObjectToView);
    m_view->setFocus();
    groupLayout->addWidget(m_view);

    setLayout(groupLayout);

    // Maximum height: 5 rows of 50 pixels + scrollbar of 50 pixels
    setMinimumHeight(300);
    setMaximumHeight(300);
}

TraceWindow::~TraceWindow()
{
    delete m_plugin;
}

void TraceWindow::reset(QDeclarativeDebugConnection *conn)
{
    if (m_plugin)
        disconnect(m_plugin,SIGNAL(complete()), this, SIGNAL(viewUpdated()));
    delete m_plugin;
    m_plugin = new TracePlugin(conn);
    connect(m_plugin,SIGNAL(complete()), this, SIGNAL(viewUpdated()));
    if (m_recordAtStart)
        m_plugin->setRecording(true);

    m_view->rootContext()->setContextProperty("connection", m_plugin);
    m_view->setSource(QUrl("qrc:/qml/MainView.qml"));

    connect(m_view->rootObject(), SIGNAL(updateCursorPosition()), this, SLOT(updateCursorPosition()));
    connect(m_view->rootObject(), SIGNAL(updateTimer()), this, SLOT(updateTimer()));
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

void TraceWindow::setRecordAtStart(bool record)
{
    m_recordAtStart = record;
}

void TraceWindow::setRecording(bool recording)
{
    m_plugin->setRecording(recording);
}

#include "tracewindow.moc"
