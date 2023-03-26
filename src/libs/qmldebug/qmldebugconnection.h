// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmldebug_global.h"

#include <QObject>
#include <QUrl>
#include <QAbstractSocket>
#include <QDataStream>

namespace QmlDebug {

class QmlDebugClient;
class QmlDebugConnectionPrivate;
class QMLDEBUG_EXPORT QmlDebugConnection : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QmlDebugConnection)
public:
    QmlDebugConnection(QObject *parent = nullptr);
    ~QmlDebugConnection() override;

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

    static constexpr int minimumDataStreamVersion()
    {
        return QDataStream::Qt_4_7;
    }

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
