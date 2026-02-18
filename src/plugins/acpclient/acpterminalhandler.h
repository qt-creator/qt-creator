// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <acp/acp.h>

#include <QHash>
#include <QJsonValue>
#include <QObject>

namespace Utils { class Process; }

namespace AcpClient::Internal {

class AcpClientObject;

class AcpTerminalHandler : public QObject
{
    Q_OBJECT

public:
    explicit AcpTerminalHandler(AcpClientObject *client, QObject *parent = nullptr);
    ~AcpTerminalHandler() override;

private:
    struct TerminalInfo
    {
        Utils::Process *process = nullptr;
        QByteArray output;
        bool exited = false;
        int exitCode = -1;
        QString signal;
        // Deferred response for wait_for_exit
        QJsonValue waitingId;
    };

    void handleCreate(const QJsonValue &id, const Acp::CreateTerminalRequest &request);
    void handleOutput(const QJsonValue &id, const Acp::TerminalOutputRequest &request);
    void handleWaitForExit(const QJsonValue &id, const Acp::WaitForTerminalExitRequest &request);
    void handleKill(const QJsonValue &id, const Acp::KillTerminalCommandRequest &request);
    void handleRelease(const QJsonValue &id, const Acp::ReleaseTerminalRequest &request);

    void sendWaitResponse(const QJsonValue &id, const TerminalInfo &info);

    AcpClientObject *m_client;
    QHash<QString, TerminalInfo> m_terminals;
    int m_nextTerminalId = 1;
};

} // namespace AcpClient::Internal
