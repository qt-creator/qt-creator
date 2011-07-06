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

#ifndef QMLPROFILERTRACECLIENT_H
#define QMLPROFILERTRACECLIENT_H

#include <qmljsdebugclient/qdeclarativedebugclient_p.h>
#include <QtCore/QStack>
#include <QtCore/QStringList>

#include "qmlprofilereventtypes.h"

namespace QmlProfiler {
namespace Internal {

struct Location
{
    Location() : line(-1) {}
    Location(const QString &file, int lineNumber) : fileName(file), line(lineNumber) {}
    QString fileName;
    int line;
};

class QmlProfilerTraceClient : public QmlJsDebugClient::QDeclarativeDebugClient
{
    Q_OBJECT
    Q_PROPERTY(bool recording READ isRecording WRITE setRecording NOTIFY recordingChanged)

public:
    QmlProfilerTraceClient(QmlJsDebugClient::QDeclarativeDebugConnection *client);

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

    bool isRecording() const { return m_recording; }

public slots:
    void setRecording(bool);
    void clearView();

signals:
    void complete();
    void gap(qint64);
    void event(int event, qint64 time);
    void range(int type, int nestingLevel, int nestingInType, qint64 startTime, qint64 length,
               const QStringList &data, const QString &fileName, int line);

    void sample(int, int, int, bool);

    void recordingChanged(bool arg);

    void enabled();
    void clear();

protected:
    virtual void statusChanged(Status);
    virtual void messageReceived(const QByteArray &);

private:
    qint64 m_inProgressRanges;
    QStack<qint64> m_rangeStartTimes[MaximumQmlEventType];
    QStack<QStringList> m_rangeDatas[MaximumQmlEventType];
    QStack<Location> m_rangeLocations[MaximumQmlEventType];
    int m_rangeCount[MaximumQmlEventType];
    qint64 m_maximumTime;
    bool m_recording;
    int m_nestingLevel;
    int m_nestingInType[MaximumQmlEventType];
};

} // namespace Internal
} // namespace QmlProfiler

#endif // QMLPROFILERTRACECLIENT_H
