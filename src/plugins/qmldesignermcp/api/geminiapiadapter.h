// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "aiapiadapter.h"

#include <QUrl>

namespace QmlDesigner {

struct AiModelInfo;

/**
 * @brief Gemini API adapter for Google's Generative Language API
 *
 * Implements Gemini-specific request/response formatting including:
 * - functionCall parts for tool calling
 * - functionResponse parts for tool results
 * - Gemini's specific "contents" and "parts" JSON structure
 * - System instructions via the system_instruction field
 */
class GeminiApiAdapter : public AiApiAdapter
{
public:
    explicit GeminiApiAdapter() = default;

    // AiApiAdapter interface
    void setRequestHeader(QNetworkRequest *request, const AiModelInfo &modelInfo) override;

    QByteArray createRequest(
        const RequestData &data,
        const AiModelInfo &modelInfo,
        const QJsonArray &tools,
        const QJsonArray &conversationHistory) override;

    QString id() const override { return "gemini"; }

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
    QUrl resolveUrl(const QString &baseUrl, const AiModelInfo &modelInfo) const override;

private:
    QJsonArray extractPartsArray(const QByteArray &response) const;
};

} // namespace QmlDesigner
