// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "aiapiadapter.h"

namespace QmlDesigner {

struct AiModelInfo;

/**
 * @brief OpenAI Responses API adapter
 *
 * Implements OpenAI-specific request/response formatting for the
 * Responses API (POST /v1/responses)
 */
class OpenAiResponseApiAdapter : public AiApiAdapter
{
public:
    explicit OpenAiResponseApiAdapter();

    // AiApiAdapter interface
    void setRequestHeader(QNetworkRequest *request, const AiModelInfo &modelInfo) override;

    QByteArray createRequest(
        const RequestData &data,
        const AiModelInfo &modelInfo,
        const QList<ToolEntry> &tools,
        const QList<ConversationMessage> &history) override;

    QString id() const override;
    QList<ToolCall> parseToolCalls(const QByteArray &response) override;
    bool isResponseComplete(const QByteArray &response) const override;
    QString extractApiError(const QByteArray &response) const override;
    QList<MessageBlock> parseResponse(const QByteArray &response) override;
    QJsonArray formatHistory(const QList<ConversationMessage> &messages) const override;
    QJsonArray formatTools(const QList<ToolEntry> &tools, bool prefixWithServer) const override;
    bool accepts(const QString &url) const override;
    void clear() override;

private:
    QString m_previousResponseId;
};

} // namespace QmlDesigner
