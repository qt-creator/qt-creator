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

#ifndef QMLPROFILERTRACECLIENT_H
#define QMLPROFILERTRACECLIENT_H

#include "qmldebugclient.h"
#include "qmlprofilereventtypes.h"
#include "qmlprofilereventlocation.h"
#include "qmldebug_global.h"

#include <QStack>
#include <QStringList>

namespace QmlDebug {

class QMLDEBUG_EXPORT QmlProfilerTraceClient : public QmlDebug::QmlDebugClient
{
    Q_OBJECT
    Q_PROPERTY(bool enabled READ isEnabled NOTIFY enabledChanged)
    Q_PROPERTY(bool recording READ isRecording WRITE setRecording NOTIFY recordingChanged)

    // don't hide by signal
    using QObject::event;

public:
    QmlProfilerTraceClient(QmlDebugConnection *client);
    ~QmlProfilerTraceClient();

    enum EventType {
        FramePaint,
        Mouse,
        Key,
        AnimationFrame,
        EndTrace,
        StartTrace,

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

    bool isEnabled() const;
    bool isRecording() const;
    void setRecording(bool);

public slots:
    void clearData();
    void sendRecordingStatus();

signals:
    void complete();
    void gap(qint64 time);
    void event(int event, qint64 time);
    void traceFinished( qint64 time );
    void traceStarted( qint64 time );
    void range(int type, int bindingType, qint64 startTime, qint64 length,
               const QStringList &data, const QmlDebug::QmlEventLocation &location);
    void frame(qint64 time, int frameRate, int animationCount);

    void recordingChanged(bool arg);

    void enabledChanged();
    void cleared();

protected:
    virtual void statusChanged(ClientStatus status);
    virtual void messageReceived(const QByteArray &);

private:
    void setRecordingFromServer(bool);

private:
    class QmlProfilerTraceClientPrivate *d;
};

} // namespace QmlDebug

#endif // QMLPROFILERTRACECLIENT_H
