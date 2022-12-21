// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
