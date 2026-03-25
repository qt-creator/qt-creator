// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <conversationturn.h>

#include <QJsonArray>
#include <QDateTime>
#include <QList>
#include <QUrl>

namespace QmlDesigner {

struct ToolResult;

/**
 * @brief Manages conversation history for agentic loops
 *
 * Stores turns as opaque role/content pairs. All API-specific formatting
 * (tool_result vs function_call_output, etc.) is the responsibility of the
 * adapter layer.
 */
class ConversationManager
{
public:
    ConversationManager();

    /**
     * @brief Add a user message to history
     */
    void addUserMessage(const QJsonArray &content);

    /**
     * @brief Add an assistant message to history
     *
     * content array taken verbatim from the response
     */
    void addAssistantMessage(const QJsonArray &content);

    /**
     * @brief Add a pre-formatted user turn carrying tool results.
     *
     * The caller (AgenticRequestManager) is responsible for building the
     * content array in the correct format for the active provider via
     * AiApiAdapter::buildToolResultsTurn().
     */
    void addToolResultsMessage(const QJsonArray &content);

    /**
     * @brief Get conversation history
     */
    QList<ConversationTurn> turns() const;

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
    int estimateTurnTokens(const ConversationTurn &turn) const;

    QList<int> m_userMessageIndices; // Indices into m_turns that are "Safe Starts"
    QList<ConversationTurn> m_turns;
    int m_maxTurns = 30; // Keep last 30 turns by default
};

} // namespace QmlDesigner
