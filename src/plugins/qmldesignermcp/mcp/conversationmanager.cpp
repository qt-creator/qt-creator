// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "conversationmanager.h"

#include <QJsonDocument>
#include <QJsonObject>

namespace QmlDesigner {

ConversationManager::ConversationManager() = default;

void ConversationManager::addUserMessage(const QJsonArray &content)
{
    m_turns.append(ConversationTurn{
        .role = "user",
        .content = content,
        .timestamp = QDateTime::currentDateTime(),
    });
    pruneIfNeeded();
}

void ConversationManager::addAssistantMessage(const QJsonArray &content)
{
    m_turns.append(ConversationTurn{
        .role = "assistant",
        .content = content,
        .timestamp = QDateTime::currentDateTime(),
    });
    pruneIfNeeded();
}

void ConversationManager::addToolResultsMessage(const QJsonArray &content)
{
    m_turns.append(ConversationTurn{
        .type = ConversationTurn::ToolResults,
        .role = "user",
        .content = content,
        .timestamp = QDateTime::currentDateTime(),
    });
    pruneIfNeeded();
}

QList<ConversationTurn> ConversationManager::turns(int maxTurns) const
{
    const int count = maxTurns > 0 ? qMin(maxTurns, m_turns.size()) : m_turns.size();
    return m_turns.sliced(m_turns.size() - count);
}

void ConversationManager::clear()
{
    m_turns.clear();
}

int ConversationManager::estimateTokenCount() const
{
    int total = 0;
    for (const ConversationTurn &turn : std::as_const(m_turns))
        total += estimateTurnTokens(turn);

    return total;
}

void ConversationManager::pruneIfNeeded()
{
    if (m_maxTurns > 0 && m_turns.size() > m_maxTurns) {
        // Remove oldest turns
        int toRemove = m_turns.size() - m_maxTurns;
        m_turns.erase(m_turns.begin(), m_turns.begin() + toRemove);
    }
}

int ConversationManager::estimateTurnTokens(const ConversationTurn &turn) const
{
    // Rough estimation: ~4 characters per token
    QString jsonStr = QString::fromUtf8(
        QJsonDocument(turn.content).toJson(QJsonDocument::Compact));
    return jsonStr.length() / 4;
}

} // namespace QmlDesigner
