// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "chatmessage.h"

namespace QmlDesigner {

ChatMessage::ChatMessage(MessageType type, const QString &content, QObject *parent)
    : QObject(parent)
    , m_type(type)
    , m_content(content)
    , m_timestamp(QDateTime::currentDateTime())
{
    m_timer.start();
}

ChatMessage::MessageType ChatMessage::type() const { return m_type; }

QString ChatMessage::content() const { return m_content; }

QString ChatMessage::toolName() const { return m_toolName; }

QString ChatMessage::serverName() const { return m_serverName; }

bool ChatMessage::success() const { return m_success; }

QDateTime ChatMessage::timestamp() const { return m_timestamp; }

QJsonObject ChatMessage::toolArgs() const { return m_toolArgs; }

void ChatMessage::setToolInfo(const QString &serverName, const QString &toolName)
{
    m_serverName = serverName;
    m_toolName = toolName;
}

void ChatMessage::setToolArgs(const QJsonObject &args)
{
    m_toolArgs = args;
}

void ChatMessage::complete(bool success) {
    m_type = success ? ToolCallCompleted : ToolCallFailed;
    m_success = success;
    const qint64 ms = m_timer.elapsed();
    if (success) {
        const QString elapsed = ms < 1000
                                    ? QObject::tr("%1ms").arg(ms)
                                    : QObject::tr("%1s").arg(ms / 1000.0, 0, 'f', 1);
        m_content += QObject::tr(" done in %1").arg(elapsed);
    } else {
        m_content += QObject::tr(" failed");
    }
}

} // namespace QmlDesigner
