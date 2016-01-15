/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#ifndef QMLPROFILERCLIENTMANAGER_H
#define QMLPROFILERCLIENTMANAGER_H

#include "qmlprofilerstatemanager.h"
#include <qmldebug/qmlprofilereventlocation.h>

#include <QObject>
#include <QStringList>
#include <QAbstractSocket>

namespace QmlProfiler {
class QmlProfilerModelManager;

namespace Internal {

class QmlProfilerClientManager : public QObject
{
    Q_OBJECT
public:
    explicit QmlProfilerClientManager(QObject *parent = 0);
    ~QmlProfilerClientManager();

    void registerProfilerStateManager(QmlProfilerStateManager *profilerState);
    void setTcpConnection(QString host, quint64 port);
    void setLocalSocket(QString file);

    void clearBufferedData();
    void discardPendingData();
    bool isConnected() const;

    void setModelManager(QmlProfilerModelManager *m);
    void setFlushInterval(quint32 flushInterval);

    bool aggregateTraces() const;
    void setAggregateTraces(bool aggregateTraces);

signals:
    void connectionFailed();
    void connectionClosed();

public slots:
    void connectTcpClient(quint16 port);
    void connectLocalClient(const QString &file);
    void disconnectClient();

private slots:
    void tryToConnect();
    void qmlDebugConnectionOpened();
    void qmlDebugConnectionClosed();
    void qmlDebugConnectionError(QAbstractSocket::SocketError error);
    void qmlDebugConnectionStateChanged(QAbstractSocket::SocketState state);
    void logState(const QString &);

    void retryMessageBoxFinished(int result);

    void qmlComplete(qint64 maximumTime);
    void qmlNewEngine(int engineId);

    void profilerStateChanged();
    void clientRecordingChanged();

private:
    class QmlProfilerClientManagerPrivate;
    QmlProfilerClientManagerPrivate *d;

    void createConnection();
    void connectClientSignals();
    void disconnectClientSignals();

    void stopClientsRecording();
};

}
}

#endif // QMLPROFILERCLIENTMANAGER_H
