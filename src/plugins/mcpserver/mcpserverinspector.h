// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <mcp/server/mcpserver.h>

#include <utils/jsonrpcinspector.h>

#include <QJsonDocument>

namespace Mcp::Internal {

using McpLogMessage = Utils::JsonRpcLogMessage;

class McpInspector : public Utils::JsonRpcInspector, public Mcp::Inspector
{
public:
    McpInspector();

    static QString sessionIdToDisplayName(const QString &sessionId);

    void onSessionStarted(const QString &) override {}
    void onSessionEnded(const QString &) override {}

    std::function<void(QByteArray)> onRequest(
        const QJsonDocument &request, const QString &sessionId) override
    {
        log(McpLogMessage::ClientMessage, sessionIdToDisplayName(sessionId), request.object());

        return [this, sessionId](const QByteArray &data) {
            log(McpLogMessage::ServerMessage,
                sessionIdToDisplayName(sessionId),
                QJsonDocument::fromJson(data).object());
        };
    }

    void onServerNotification(const QJsonDocument &notification, const QString &sessionId) override
    {
        log(McpLogMessage::ServerMessage, sessionIdToDisplayName(sessionId), notification.object());
    }

    void onClientNotification(const QJsonDocument &notification, const QString &sessionId) override
    {
        log(McpLogMessage::ClientMessage, sessionIdToDisplayName(sessionId), notification.object());
    }
};

} // namespace Mcp::Internal
