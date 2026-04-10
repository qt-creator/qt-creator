// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <QDateTime>
#include <QJsonObject>
#include <QList>
#include <QString>
#include <QUrl>

namespace QmlDesigner {

/**
 * @brief A single provider-agnostic block within a conversation message
 *
 * Only the fields relevant to the block's Type are populated.
 */
struct MessageBlock
{
    enum class Type {
        Text,
        Image,
        ToolUse,
        ToolResult,
    };

    Type type = Type::Text;

    // Type::Text
    QString text;

    // Type::Image
    QUrl url;

    // Type::ToolUse
    QString id;
    QString serverName;
    QString toolName;
    QJsonObject arguments;

    // Type::ToolResult
    QString toolCallId;  // Matches the ToolUse::id this result belongs to
    QString content;
    QString error;
    bool success = false;

    QString auxiliaryData; // provider-specific extras
};

/**
 * @brief A single turn in the conversation, stored in provider-agnostic form.
 */
struct ConversationMessage
{
    enum class Role {
        User,
        Assistant,
        ToolResults, // Tool results are logically a user-role turn but kept distinct so adapters
                     // can format them differently (e.g. Claude wraps them in a user message,
                     // Gemini uses functionResponse parts).
    };

    Role role = Role::User;
    QList<MessageBlock> blocks;
    QDateTime timestamp;
};

} // namespace QmlDesigner
