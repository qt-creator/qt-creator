// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "conversationmanager.h"

namespace QmlDesigner {

ConversationManager::ConversationManager() = default;

void ConversationManager::addUserMessage(const QList<MessageBlock> &blocks)
{
    m_userMessageIndices.append(m_messages.size());

    m_messages.append(ConversationMessage{
        .role = ConversationMessage::Role::User,
        .blocks = blocks,
        .timestamp = QDateTime::currentDateTime(),
    });
    pruneIfNeeded();
}

void ConversationManager::addAssistantMessage(const QList<MessageBlock> &blocks)
{
    m_messages.append(ConversationMessage{
        .role = ConversationMessage::Role::Assistant,
        .blocks = blocks,
        .timestamp = QDateTime::currentDateTime(),
    });
    pruneIfNeeded();
}

void ConversationManager::addToolResultsMessage(const QList<MessageBlock> &blocks)
{
    m_messages.append(ConversationMessage{
        .role = ConversationMessage::Role::ToolResults,
        .blocks = blocks,
        .timestamp = QDateTime::currentDateTime(),
    });
    pruneIfNeeded();
}

QList<ConversationMessage> ConversationManager::messages() const
{
    return m_messages;
}

void ConversationManager::clear()
{
    m_userMessageIndices.clear();
    m_messages.clear();
}

int ConversationManager::estimateTokenCount() const
{
    int total = 0;
    for (const ConversationMessage &message : std::as_const(m_messages))
        total += estimateMessageTokens(message);
    return total;
}

void ConversationManager::pruneIfNeeded()
{
    if (m_maxTurns <= 0 || m_messages.size() <= m_maxTurns)
        return;

    int cutAt = 0;

    // Keep jumping to the next user index until the remaining size is safe
    while (m_userMessageIndices.size() > 1 && (m_messages.size() - m_userMessageIndices.at(1)) >= m_maxTurns) {
        m_userMessageIndices.removeFirst();
        cutAt = m_userMessageIndices.first();
    }

    if (cutAt > 0) {
        m_messages.erase(m_messages.begin(), m_messages.begin() + cutAt);
        for (int &idx : m_userMessageIndices)
            idx -= cutAt;
    }
}

int ConversationManager::estimateMessageTokens(const ConversationMessage &message) const
{
    // Rough estimate: count text characters across all text blocks, ~4 chars per token
    int chars = 0;
    for (const MessageBlock &block : message.blocks) {
        if (block.type == MessageBlock::Type::Text)
            chars += block.text.size();
        else if (block.type == MessageBlock::Type::ToolResult)
            chars += block.content.size() + block.error.size();
    }
    return chars / 4;
}

} // namespace QmlDesigner
