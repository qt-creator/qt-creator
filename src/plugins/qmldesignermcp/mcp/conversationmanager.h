// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <QJsonArray>
#include <QDateTime>
#include <QList>
#include <QUrl>

namespace QmlDesigner {

struct ToolResult;

/**
 * @brief Manages conversation history for agentic loops
 *
 * - Turn tracking (user/assistant messages)
 * - History pruning to respect context windows
 * - Token estimation
 */
class ConversationManager
{
public:
    struct Turn {
        QString role;           // "user" or "assistant"
        QJsonArray content;     // Message content
        QDateTime timestamp;
    };

    ConversationManager();

    /**
     * @brief Add user message to history
     */
    void addUserMessage(const QString &text, const QUrl &attachedImageUrl = {});

    /**
     * @brief Add assistant message to history
     */
    void addAssistantMessage(const QJsonArray &content);

    /**
     * @brief Add tool results as a user message
     */
    void addToolResults(const QList<ToolResult> &results);

    /**
     * @brief Get conversation history formatted for API
     *
     * Automatically prunes old turns if exceeding max turns.
     *
     * @param maxTurns Maximum number of turns to include (0 = all)
     * @return JSON array of messages
     */
    QJsonArray getHistory(int maxTurns = 0) const;

    /**
     * @brief Clear all conversation history
     */
    void clear();

    /**
     * @brief Get number of turns
     */
    int turnCount() const { return m_turns.size(); }

    /**
     * @brief Get estimated token count (rough approximation)
     */
    int estimateTokenCount() const;

    /**
     * @brief Set maximum turns to keep (auto-prunes on add)
     */
    void setMaxTurns(int max) { m_maxTurns = max; }

private:
    void pruneIfNeeded();
    int estimateTurnTokens(const Turn &turn) const;

    QList<Turn> m_turns;
    int m_maxTurns = 20; // Keep last 20 turns by default
};

} // namespace QmlDesigner
