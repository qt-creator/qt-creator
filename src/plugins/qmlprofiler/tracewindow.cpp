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

#include <QtCore/qdebug.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qdatastream.h>
#include <QtCore/qmargins.h>

#include <QtGui/qapplication.h>
#include <QtGui/qpainter.h>
#include <QtGui/qboxlayout.h>
#include <QtGui/qevent.h>
#include <QtCore/qstack.h>

#include <QGraphicsObject>

#include <QtDeclarative/qdeclarativeview.h>
#include <QtDeclarative/qdeclarativecontext.h>
#include <QtDeclarative/qdeclarative.h>

#include "qmlprofilerplugin.h"
//#include <jsdebuggeragent.h>
//#include <qdeclarativeviewobserver.h>

#define GAP_TIME 150

using QmlJsDebugClient::QDeclarativeDebugClient;

namespace QmlProfiler {
namespace Internal {

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
    void clearView();

signals:
    void complete();
    void gap(qint64);
    void event(int event, qint64 time);
    void range(int type, qint64 startTime, qint64 length, const QStringList &data, const QString &fileName, int line);

    void sample(int, int, int, bool);

    void recordingChanged(bool arg);

    void enabled();
    void clear();

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

} // namespace Internal
} // namespace QmlProfiler

using namespace QmlProfiler::Internal;

TracePlugin::TracePlugin(QDeclarativeDebugConnection *client)
    : QDeclarativeDebugClient(QLatin1String("CanvasFrameRate"), client), m_inProgressRanges(0), m_maximumTime(0), m_recording(false)
{
    ::memset(m_rangeCount, 0, MaximumRangeType * sizeof(int));
}

void TracePlugin::clearView()
{
    ::memset(m_rangeCount, 0, MaximumRangeType * sizeof(int));
    emit clear();
}

void TracePlugin::setRecording(bool v)
{
    if (v == m_recording)
        return;

    if (status() == Enabled) {
        QByteArray ba;
        QDataStream stream(&ba, QIODevice::WriteOnly);
        stream << v;
        sendMessage(ba);
    }

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
: QWidget(parent)
{
    setObjectName(tr("QML Performance Monitor"));

    QVBoxLayout *groupLayout = new QVBoxLayout;
    groupLayout->setContentsMargins(0, 0, 0, 0);
    groupLayout->setSpacing(0);

    m_view = new QDeclarativeView(this);

    if (QmlProfilerPlugin::debugOutput) {
        //new QmlJSDebugger::JSDebuggerAgent(m_view->engine());
        //new QmlJSDebugger::QDeclarativeViewObserver(m_view, m_view);
    }

    m_view->setResizeMode(QDeclarativeView::SizeRootObjectToView);
    m_view->setFocus();
    groupLayout->addWidget(m_view);

    setLayout(groupLayout);

    // Maximum height: 5 rows of 50 pixels + scrollbar of 50 pixels
    setFixedHeight(300);
}

TraceWindow::~TraceWindow()
{
    delete m_plugin.data();
}

void TraceWindow::reset(QDeclarativeDebugConnection *conn)
{
    if (m_plugin)
        disconnect(m_plugin.data(), SIGNAL(complete()), this, SIGNAL(viewUpdated()));
    delete m_plugin.data();
    m_plugin = new TracePlugin(conn);
    connect(m_plugin.data(), SIGNAL(complete()), this, SIGNAL(viewUpdated()));
    connect(m_plugin.data(), SIGNAL(range(int,qint64,qint64,QStringList,QString,int)),
            this, SIGNAL(range(int,qint64,qint64,QStringList,QString,int)));

    m_view->rootContext()->setContextProperty("connection", m_plugin.data());
    m_view->setSource(QUrl("qrc:/qmlprofiler/MainView.qml"));

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

void TraceWindow::clearDisplay()
{
    if (m_plugin)
        m_plugin.data()->clearView();
}

void TraceWindow::setRecording(bool recording)
{
    if (m_plugin)
        m_plugin.data()->setRecording(recording);
}

bool TraceWindow::isRecording() const
{
    return (m_plugin.data()->recording());
}

#include "tracewindow.moc"
