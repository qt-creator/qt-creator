// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "acpprotocoladapter.h"

namespace AcpClient::Internal {

// Speaks the ACP v2 wire protocol. Mostly a thin passthrough; the turn is
// finished by an idle state_update, not by the session/prompt response.
class AcpProtocolV2Adapter : public AcpProtocolAdapter
{
    Q_OBJECT

public:
    AcpProtocolV2Adapter(AcpClientObject *client, const Acp::V2::InitializeResponse &response,
                         QObject *parent = nullptr);

    int protocolVersion() const override { return 2; }

    void newSession(const QString &cwd, const QJsonArray &mcpServers) override;
    void resumeSession(const QString &sessionId, const QString &cwd,
                       const QJsonArray &mcpServers) override;
    void listSessions(const std::optional<QString> &cursor) override;
    void deleteSession(const QString &sessionId) override;
    void closeSession(const QString &sessionId) override;
    void prompt(const QString &sessionId, const QJsonArray &content) override;
    void cancel(const QString &sessionId) override;
    void authenticate(const QString &methodId) override;
    void setConfigOption(const QString &sessionId, const QString &configId,
                         const QJsonValue &value) override;

    bool supportsSessionList() const override;
    bool supportsSessionDelete() const override;
    bool supportsSessionClose() const override;
    bool supportsImagePrompt() const override;
    bool supportsEmbeddedContext() const override;
    QList<Acp::V2::AuthMethod> authMethods() const override;

private:
    void finishTurn(const std::optional<Acp::V2::StopReason> &stopReason);
    void handleNotification(const QString &method, const QJsonObject &params);
    void handleRequest(const QJsonValue &id, const QString &method, const QJsonObject &params);
    void emitConfigOptions(const QJsonArray &options);

    Acp::V2::InitializeResponse m_initializeResponse;
    bool m_turnActive = false;
};

} // namespace AcpClient::Internal
