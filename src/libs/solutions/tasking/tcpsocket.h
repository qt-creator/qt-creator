// Copyright (C) 2024 Jarek Kobus
// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef TASKING_TCPSOCKET_H
#define TASKING_TCPSOCKET_H

#include "tasking_global.h"

#include "tasktree.h"

#include <QtNetwork/QTcpSocket>

#include <memory>

QT_BEGIN_NAMESPACE

namespace Tasking {

// This class introduces the dependency to Qt::Network, otherwise Tasking namespace
// is independent on Qt::Network.
// Possibly, it could be placed inside Qt::Network library, as a wrapper around QTcpSocket.

class TASKING_EXPORT TcpSocket final : public QObject
{
    Q_OBJECT

public:
    ~TcpSocket();
    void setAddress(const QHostAddress &address) { m_address = address; }
    void setPort(quint16 port) { m_port = port; }
    void setWriteData(const QByteArray &data) { m_writeData = data; }
    QTcpSocket *socket() const { return m_socket.get(); }
    void start();

Q_SIGNALS:
    void started();
    void done(DoneResult result);

private:
    QHostAddress m_address;
    quint16 m_port = 0;
    QByteArray m_writeData;
    std::unique_ptr<QTcpSocket> m_socket;
    QAbstractSocket::SocketError m_error = QAbstractSocket::UnknownSocketError;
};

using TcpSocketTask = SimpleCustomTask<TcpSocket>;

} // namespace Tasking

QT_END_NAMESPACE

#endif // TASKING_TCPSOCKET_H
