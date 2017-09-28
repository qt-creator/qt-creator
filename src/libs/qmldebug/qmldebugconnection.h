/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#pragma once

#include "qmldebug_global.h"

#include <QObject>
#include <QUrl>
#include <QAbstractSocket>

namespace QmlDebug {

class QmlDebugClient;
class QmlDebugConnectionPrivate;
class QMLDEBUG_EXPORT QmlDebugConnection : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(QmlDebugConnection)
    Q_DECLARE_PRIVATE(QmlDebugConnection)
public:
    QmlDebugConnection(QObject * = 0);
    ~QmlDebugConnection();

    void connectToHost(const QString &hostName, quint16 port);
    void startLocalServer(const QString &fileName);
    QAbstractSocket::SocketState socketState() const;

    int currentDataStreamVersion() const;
    void setMaximumDataStreamVersion(int maximumVersion);

    bool isConnected() const;
    bool isConnecting() const;
    void close();

    QmlDebugClient *client(const QString &name) const;
    bool addClient(const QString &name, QmlDebugClient *client);
    bool removeClient(const QString &name);

    float serviceVersion(const QString &serviceName) const;
    bool sendMessage(const QString &name, const QByteArray &message);

    static int minimumDataStreamVersion();

signals:
    void connected();
    void disconnected();
    void connectionFailed();

    void logError(const QString &error);
    void logStateChange(const QString &state);

private:
    void newConnection();
    void socketConnected();
    void socketDisconnected();
    void protocolReadyRead();

    QScopedPointer<QmlDebugConnectionPrivate> d_ptr;
};

} // namespace QmlDebug
