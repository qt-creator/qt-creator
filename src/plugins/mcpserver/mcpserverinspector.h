// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <mcp/server/mcpserver.h>

#include "mcpservertr.h"

#include <QJsonObject>
#include <QJsonValue>
#include <QMap>
#include <QObject>
#include <QTime>

#include <list>
#include <optional>

namespace Mcp::Internal {

class McpLogMessage
{
public:
    enum MessageSender { ClientMessage, ServerMessage };

    McpLogMessage() = default;
    McpLogMessage(MessageSender sender, const QTime &time, const QJsonObject &message);

    MessageSender sender = ClientMessage;
    QTime time;
    QJsonObject message;

    QJsonValue id() const;
    QString displayText() const;

private:
    mutable std::optional<QJsonValue> m_id;
    mutable std::optional<QString> m_displayText;
};

class McpInspector : public QObject, public Mcp::Inspector
{
    Q_OBJECT
public:
    McpInspector() = default;

    void show(const QString &defaultClient = {});

    void log(
        McpLogMessage::MessageSender sender, const QString &sessionId, const QJsonObject &message);

    std::list<McpLogMessage> messages(const QString &sessionId) const;
    QStringList sessions() const;
    void clear() { m_logs.clear(); }

    void onSessionStarted(const QString &) override {}

    void onSessionEnded(const QString &) override {}

    static QString sessionIdToDisplayName(const QString &sessionId)
    {
        return sessionId.isEmpty() ? Tr::tr("Global") : sessionId;
    }

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

signals:
    void newMessage(const QString &sessionId, const McpLogMessage &message);

private:
    void onInspectorClosed();

    QMap<QString, std::list<McpLogMessage>> m_logs;
    QWidget *m_currentWidget = nullptr;
    int m_logSize = 100;
};

} // namespace Mcp::Internal
