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

#include <launcherpackets.h>

#include <QByteArray>
#include <QHash>
#include <QObject>

QT_BEGIN_NAMESPACE
class QLocalSocket;
QT_END_NAMESPACE

namespace Utils {
namespace Internal {
class Process;

class LauncherSocketHandler : public QObject
{
    Q_OBJECT
public:
    explicit LauncherSocketHandler(QString socketPath, QObject *parent = nullptr);
    ~LauncherSocketHandler() override;

    void start();

private:
    void handleSocketData();
    void handleSocketError();
    void handleSocketClosed();

    void handleProcessStarted(Process *process);
    void handleProcessError(Process *process);
    void handleProcessFinished(Process *process);
    void handleReadyReadStandardOutput(Process *process);
    void handleReadyReadStandardError(Process *process);

    void handleStartPacket();
    void handleWritePacket();
    void handleStopPacket();
    void handleShutdownPacket();

    void sendPacket(const LauncherPacket &packet);

    Process *setupProcess(quintptr token);
    void removeProcess(quintptr token);

    const QString m_serverPath;
    QLocalSocket * const m_socket;
    PacketParser m_packetParser;
    QHash<quintptr, Process *> m_processes;
};

} // namespace Internal
} // namespace Utils
