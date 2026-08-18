// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "acpprotocoladapter.h"

#include <acp/acp.h>

namespace AcpClient::Internal {

// Speaks the ACP v1 wire protocol and up-converts payloads to the v2
// vocabulary at the JSON level. Session modes are represented as a
// synthesized select config option with category "mode".
class AcpProtocolV1Adapter : public AcpProtocolAdapter
{
    Q_OBJECT

public:
    AcpProtocolV1Adapter(AcpClientObject *client, const Acp::InitializeResponse &response,
                         QObject *parent = nullptr);

    int protocolVersion() const override { return 1; }

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
    void handleNotification(const QString &method, const QJsonObject &params);
    void handlePermissionRequest(const QJsonValue &id,
                                 const Acp::RequestPermissionRequest &request);
    void applySessionSetup(const QJsonObject &result);
    QList<Acp::V2::SessionConfigOption> convertConfigOptions(const QJsonArray &options) const;
    std::optional<Acp::V2::SessionConfigOption> synthesizedModeOption() const;
    QList<Acp::V2::SessionConfigOption> fullConfigOptions() const;
    QJsonArray fullConfigOptionsJson() const;

    Acp::InitializeResponse m_initializeResponse;
    QList<Acp::V2::SessionConfigOption> m_configOptions;
    QList<QJsonObject> m_modes;
    QString m_currentModeId;
    int m_turnCounter = 0;
};

} // namespace AcpClient::Internal
