// Copyright (C) 2024 Jarek Kobus
// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "tcpsocket.h"

QT_BEGIN_NAMESPACE

namespace Tasking {

void TcpSocket::start()
{
    if (m_socket) {
        qWarning("The TcpSocket is already running. Ignoring the call to start().");
        return;
    }
    if (m_address.isNull()) {
        qWarning("Can't start the TcpSocket with invalid address. "
                 "Stopping with an error.");
        m_error = QAbstractSocket::HostNotFoundError;
        emit done(DoneResult::Error);
        return;
    }

    m_socket.reset(new QTcpSocket);
    connect(m_socket.get(), &QAbstractSocket::errorOccurred, this,
            [this](QAbstractSocket::SocketError error) {
        m_error = error;
        m_socket->disconnect();
        emit done(DoneResult::Error);
        m_socket.release()->deleteLater();
    });
    connect(m_socket.get(), &QAbstractSocket::connected, this, [this] {
        if (!m_writeData.isEmpty())
            m_socket->write(m_writeData);
        emit started();
    });
    connect(m_socket.get(), &QAbstractSocket::disconnected, this, [this] {
        m_socket->disconnect();
        emit done(DoneResult::Success);
        m_socket.release()->deleteLater();
    });

    m_socket->connectToHost(m_address, m_port);
}

TcpSocket::~TcpSocket()
{
    if (m_socket) {
        m_socket->disconnect();
        m_socket->abort();
    }
}

} // namespace Tasking

QT_END_NAMESPACE
