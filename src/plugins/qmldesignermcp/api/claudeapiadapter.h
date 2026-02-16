// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "aiapiadapter.h"

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
        const QJsonArray &tools,
        const QJsonArray &conversationHistory) override;

    QList<ToolCall> parseToolCalls(const QByteArray &response) override;
    bool isResponseComplete(const QByteArray &response) const override;
    AiResponse interpretResponse(const QByteArray &response) override;
    QJsonObject buildAssistantTurn(const QByteArray &response) override;
    QJsonObject buildToolResultsTurn(const QList<ToolResult> &results) override;
    bool accepts(const QUrl &url) const override;

private:
    QJsonArray extractContentArray(const QByteArray &response) const;
    QString extractTextFromContent(const QJsonArray &content) const;
};

} // namespace QmlDesigner
