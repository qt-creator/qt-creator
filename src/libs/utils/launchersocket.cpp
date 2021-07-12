/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "launchersocket.h"

#include "qtcassert.h"

#include <QtCore/qcoreapplication.h>
#include <QtCore/qtimer.h>
#include <QtNetwork/qlocalsocket.h>

namespace Utils {
namespace Internal {

LauncherSocket::LauncherSocket(QObject *parent) : QObject(parent)
{
    qRegisterMetaType<Utils::Internal::LauncherPacketType>();
    qRegisterMetaType<quintptr>("quintptr");
}

void LauncherSocket::sendData(const QByteArray &data)
{
    if (!isReady())
        return;
    std::lock_guard<std::mutex> locker(m_requestsMutex);
    m_requests.push_back(data);
    if (m_requests.size() == 1)
        QTimer::singleShot(0, this, &LauncherSocket::handleRequests);
}

void LauncherSocket::shutdown()
{
    const auto socket = m_socket.exchange(nullptr);
    if (!socket)
        return;
    socket->disconnect();
    socket->write(ShutdownPacket().serialize());
    socket->waitForBytesWritten(1000);
    socket->deleteLater();
}

void LauncherSocket::setSocket(QLocalSocket *socket)
{
    QTC_ASSERT(!m_socket, return);
    m_socket.store(socket);
    m_packetParser.setDevice(m_socket);
    connect(m_socket,
#if (QT_VERSION < QT_VERSION_CHECK(5, 15, 0))
            static_cast<void(QLocalSocket::*)(QLocalSocket::LocalSocketError)>(&QLocalSocket::error),
#else
            &QLocalSocket::errorOccurred,
#endif
            this, &LauncherSocket::handleSocketError);
    connect(m_socket, &QLocalSocket::readyRead,
            this, &LauncherSocket::handleSocketDataAvailable);
    connect(m_socket, &QLocalSocket::disconnected,
            this, &LauncherSocket::handleSocketDisconnected);
    emit ready();
}

void LauncherSocket::handleSocketError()
{
    auto socket = m_socket.load();
    if (socket->error() != QLocalSocket::PeerClosedError)
        handleError(QCoreApplication::translate("Utils::LauncherSocket",
                    "Socket error: %1").arg(socket->errorString()));
}

void LauncherSocket::handleSocketDataAvailable()
{
    try {
        if (!m_packetParser.parse())
            return;
    } catch (const PacketParser::InvalidPacketSizeException &e) {
        handleError(QCoreApplication::translate("Utils::LauncherSocket",
                    "Internal protocol error: invalid packet size %1.").arg(e.size));
        return;
    }
    switch (m_packetParser.type()) {
    case LauncherPacketType::ProcessError:
    case LauncherPacketType::ProcessStarted:
    case LauncherPacketType::ProcessFinished:
        emit packetArrived(m_packetParser.type(), m_packetParser.token(),
                           m_packetParser.packetData());
        break;
    default:
        handleError(QCoreApplication::translate("Utils::LauncherSocket",
                    "Internal protocol error: invalid packet type %1.")
                    .arg(static_cast<int>(m_packetParser.type())));
        return;
    }
    handleSocketDataAvailable();
}

void LauncherSocket::handleSocketDisconnected()
{
    handleError(QCoreApplication::translate("Utils::LauncherSocket",
                "Launcher socket closed unexpectedly"));
}

void LauncherSocket::handleError(const QString &error)
{
    const auto socket = m_socket.exchange(nullptr);
    socket->disconnect();
    socket->deleteLater();
    emit errorOccurred(error);
}

void LauncherSocket::handleRequests()
{
    const auto socket = m_socket.load();
    QTC_ASSERT(socket, return);
    std::lock_guard<std::mutex> locker(m_requestsMutex);
    for (const QByteArray &request : qAsConst(m_requests))
        socket->write(request);
    m_requests.clear();
}

} // namespace Internal
} // namespace Utils
