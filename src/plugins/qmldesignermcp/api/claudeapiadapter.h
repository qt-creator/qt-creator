// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "aiapiadapter.h"

#include <QUrl>

namespace QmlDesigner {

struct AiModelInfo;

/**
 * @brief Claude API adapter for Anthropic's Messages API
 *
 * Implements Claude-specific request/response formatting including:
 * - tool_use blocks for tool calling
 * - tool_result blocks for tool responses
 * - Claude's specific JSON structure
 */
class ClaudeApiAdapter : public AiApiAdapter
{
public:
    explicit ClaudeApiAdapter();

    // AiApiAdapter interface
    void setRequestHeader(QNetworkRequest *request, const AiModelInfo &modelInfo) override;

    QByteArray createRequest(
        const RequestData &data,
        const AiModelInfo &modelInfo,
        const QList<ToolEntry> &tools,
        const QList<ConversationTurn> &history) override;

    QString id() const override;
    QList<ToolCall> parseToolCalls(const QByteArray &response) override;
    bool isResponseComplete(const QByteArray &response) const override;
    AiResponse interpretResponse(const QByteArray &response) override;
    QJsonArray buildUserMessage(const QString &text, const QUrl &imageUrl = {}) override;
    QJsonArray buildAssistantTurn(const QByteArray &response) override;
    QJsonArray buildToolResultsTurn(const QList<ToolResult> &results) override;
    QJsonArray formatHistory(const QList<ConversationTurn> &turns) const override;
    QJsonArray formatTools(const QList<ToolEntry> &tools, bool prefixWithServer) const override;
    QString extractText(const QByteArray &response) const override;
    bool accepts(const QString &url) const override;

private:
    QJsonArray extractContentArray(const QByteArray &response) const;
    QString extractTextFromContent(const QJsonArray &content) const;
};

} // namespace QmlDesigner
