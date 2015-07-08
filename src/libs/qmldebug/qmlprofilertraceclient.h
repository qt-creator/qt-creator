/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
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
    QmlProfilerTraceClient(QmlDebugConnection *client, quint64 features);
    ~QmlProfilerTraceClient();

    bool isEnabled() const;
    bool isRecording() const;
    void setRecording(bool);
    quint64 recordedFeatures() const;

public slots:
    void clearData();
    void sendRecordingStatus(int engineId = -1);
    void setRequestedFeatures(quint64 features);

signals:
    void complete(qint64 maximumTime);
    void gap(qint64 time);
    void traceFinished(qint64 time, const QList<int> &engineIds);
    void traceStarted(qint64 time, const QList<int> &engineIds);
    void rangedEvent(QmlDebug::Message, QmlDebug::RangeType, int detailType, qint64 startTime,
                     qint64 length, const QString &data,
                     const QmlDebug::QmlEventLocation &location, qint64 param1, qint64 param2,
                     qint64 param3, qint64 param4, qint64 param5);
    void recordingChanged(bool arg);
    void recordedFeaturesChanged(quint64 features);

    void enabledChanged();
    void cleared();

protected:
    virtual void stateChanged(State status);
    virtual void messageReceived(const QByteArray &);

private:
    void setRecordingFromServer(bool);

private:
    class QmlProfilerTraceClientPrivate *d;
};

} // namespace QmlDebug

#endif // QMLPROFILERTRACECLIENT_H
