// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "conversationmessage.h"

#include <QList>

namespace QmlDesigner {

struct ToolResult;

/**
 * @brief Manages the provider-agnostic conversation history.
 *
 * Stores ConversationMessage objects. Adapters are responsible for converting to/from
 * provider-specific JSON
 */
class ConversationManager
{
public:
    ConversationManager();

    void addUserMessage(const QList<MessageBlock> &blocks);
    void addAssistantMessage(const QList<MessageBlock> &blocks);
    void addToolResultsMessage(const QList<MessageBlock> &blocks);

    QList<ConversationMessage> messages() const;

    void clear();
    int estimateTokenCount() const;

    void setMaxTurns(int max) { m_maxTurns = max; }

private:
    void pruneIfNeeded();
    int estimateMessageTokens(const ConversationMessage &message) const;

    QList<ConversationMessage> m_messages;
    QList<int> m_userMessageIndices; // Indices into m_messages that are "Safe Starts"
    int m_maxTurns = 30; // Keep last 30 turns by default
};

} // namespace QmlDesigner
