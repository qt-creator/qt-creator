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

#include "qmlprofilertraceclient.h"

using namespace QmlProfiler::Internal;
using namespace QmlJsDebugClient;

static const int GAP_TIME = 150;

QmlProfilerTraceClient::QmlProfilerTraceClient(QDeclarativeDebugConnection *client)
    : QDeclarativeDebugClient(QLatin1String("CanvasFrameRate"), client),
      m_inProgressRanges(0), m_maximumTime(0), m_recording(false), m_nestingLevel(0)
{
    ::memset(m_rangeCount, 0, MaximumQmlEventType * sizeof(int));
    ::memset(m_nestingInType, 0, MaximumQmlEventType * sizeof(int));
}

void QmlProfilerTraceClient::clearView()
{
    ::memset(m_rangeCount, 0, MaximumQmlEventType * sizeof(int));
    ::memset(m_nestingInType, 0, MaximumQmlEventType * sizeof(int));
    m_nestingLevel = 0;
    emit clear();
}

void QmlProfilerTraceClient::setRecording(bool v)
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

void QmlProfilerTraceClient::statusChanged(Status status)
{
    if (status == Enabled) {
        m_recording = !m_recording;
        setRecording(!m_recording);
        emit enabled();
    }
}

void QmlProfilerTraceClient::messageReceived(const QByteArray &data)
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

        if (range >= MaximumQmlEventType)
            return;

        if (messageType == RangeStart) {
            m_rangeStartTimes[range].push(time);
            m_inProgressRanges |= (static_cast<qint64>(1) << range);
            ++m_rangeCount[range];
            ++m_nestingLevel;
            ++m_nestingInType[range];
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
                if (m_inProgressRanges & (static_cast<qint64>(1) << range))
                    m_inProgressRanges &= ~(static_cast<qint64>(1) << range);

                m_maximumTime = qMax(time, m_maximumTime);
                QStringList data = m_rangeDatas[range].count() ? m_rangeDatas[range].pop() : QStringList();
                Location location = m_rangeLocations[range].count() ? m_rangeLocations[range].pop() : Location();

                qint64 startTime = m_rangeStartTimes[range].pop();
                emit this->range((QmlEventType)range, m_nestingLevel, m_nestingInType[range], startTime,
                                 time - startTime, data, location.fileName, location.line);
                --m_nestingLevel;
                --m_nestingInType[range];
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
