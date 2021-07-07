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

#pragma once

#include "launcherpackets.h"

#include <QtCore/qobject.h>

#include <mutex>
#include <vector>

QT_BEGIN_NAMESPACE
class QLocalSocket;
QT_END_NAMESPACE

namespace Utils {
class LauncherInterface;

namespace Internal {

class LauncherSocket : public QObject
{
    Q_OBJECT
    friend class Utils::LauncherInterface;
public:
    bool isReady() const { return m_socket.load(); }
    void sendData(const QByteArray &data);

signals:
    void ready();
    void errorOccurred(const QString &error);
    void packetArrived(Utils::Internal::LauncherPacketType type, quintptr token,
                       const QByteArray &payload);

private:
    LauncherSocket(QObject *parent);

    void setSocket(QLocalSocket *socket);
    void shutdown();

    void handleSocketError();
    void handleSocketDataAvailable();
    void handleSocketDisconnected();
    void handleError(const QString &error);
    void handleRequests();

    std::atomic<QLocalSocket *> m_socket{nullptr};
    PacketParser m_packetParser;
    std::vector<QByteArray> m_requests;
    std::mutex m_requestsMutex;
};

} // namespace Internal
} // namespace Utils
