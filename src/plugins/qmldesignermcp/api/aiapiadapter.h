// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <conversationturn.h>

#include <QByteArray>
#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QUrl>

QT_FORWARD_DECLARE_CLASS(QNetworkRequest)

namespace QmlDesigner {

struct AiModelInfo;
class AiResponse;
struct RequestData;
struct ToolCall;
struct ToolResult;

/**
 * @brief Base interface for AI provider adapters
 *
 * Each AI provider (Claude, OpenAI, etc.) implements this interface
 * to handle provider-specific request/response formatting.
 */
class AiApiAdapter
{
public:
    virtual ~AiApiAdapter() = default;

    /**
     * @brief Set HTTP headers for the request
     */
    virtual void setRequestHeader(QNetworkRequest *request, const AiModelInfo &modelInfo) = 0;

    /**
     * @brief Create request body with tools and conversation history
     *
     * @param data User's request data
     * @param modelInfo Model configuration
     * @param tools Available MCP tools (already formatted for this provider)
     * @param conversationHistory Message history
     * @return JSON request body
     */
    virtual QByteArray createRequest(
        const RequestData &data,
        const AiModelInfo &modelInfo,
        const QJsonArray &tools,
        const QJsonArray &conversationHistory) = 0;

    /**
     * @brief id for this adapter
     */
    virtual QString id() const = 0;

    /**
     * @brief Parse LLM response to extract tool calls
     *
     * @return List of tool calls, or empty if none requested
     */
    virtual QList<ToolCall> parseToolCalls(const QByteArray &response) = 0;

    /**
     * @brief Check if response is complete (no more tool calls needed)
     */
    virtual bool isResponseComplete(const QByteArray &response) const = 0;

    /**
    * @brief Convert response to AiResponse object
    *
    * Parses and extracts the text content and reports parsing errors.
    */
    virtual AiResponse interpretResponse(const QByteArray &response) = 0;

    /**
     * @brief Format conversation history for the provider
     *
     * Converts the internal list of ConversationTurn objects into the
     * provider-specific JSON format expected by the API. This typically
     * expands each turn into one or more message entries (user, assistant,
     * tool results, etc.) depending on how the provider structures
     * conversations.
     *
     * @param turns Conversation history stored by the client
     * @return JSON array representing the formatted message history
     */
    virtual QJsonArray formatHistory(const QList<ConversationTurn> &turns) const = 0;
    /**
     * @brief Build conversation turn from user message
     *
     * @return JSON object representing user's message
     */
    virtual QJsonArray buildUserMessage(const QString &text, const QUrl &imageUrl = {}) = 0;

    /**
     * @brief Build conversation turn from assistant response
     *
     * @return JSON object representing assistant's message
     */
    virtual QJsonArray buildAssistantTurn(const QByteArray &response) = 0;

    /**
     * @brief Build conversation turn from tool results
     *
     * @return JSON object representing tool results as user message
     */
    virtual QJsonArray buildToolResultsTurn(const QList<ToolResult> &results) = 0;

    /**
     * @brief Extract plain text from a raw LLM response body.
     *
     * Extract text blocks in the response, ignoring tool_use and
     * other non-text content types. Used to surface the model's reasoning
     * text that may accompany tool calls.
     */
    virtual QString extractText(const QByteArray &response) const = 0;

    /**
     * @brief Check if this adapter can handle the given provider's url
     */
    virtual bool accepts(const QUrl &url) const = 0;

    /**
     * @brief Clear any current state stored in the adapter
     */
    virtual void clear() {};

};

} // namespace QmlDesigner
