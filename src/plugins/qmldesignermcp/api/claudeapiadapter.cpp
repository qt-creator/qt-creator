// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "claudeapiadapter.h"

#include "agenticrequestmanager.h"
#include "aiproviderconfig.h"
#include "airesponse.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>

namespace QmlDesigner {

ClaudeApiAdapter::ClaudeApiAdapter()
{}

void ClaudeApiAdapter::setRequestHeader(QNetworkRequest *request, const AiModelInfo &modelInfo)
{
    request->setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request->setRawHeader("x-api-key", modelInfo.apiKey.toUtf8());
    request->setRawHeader("anthropic-version", "2023-06-01");
}

QByteArray ClaudeApiAdapter::createRequest(
    const RequestData &data,
    const AiModelInfo &modelInfo,
    const QJsonArray &tools,
    const QJsonArray &conversationHistory)
{
    QString systemPrompt = data.manifest;

    if (!data.projectStructure.isEmpty()) {
        systemPrompt += "\n\n<project_structure>\n"
                        + data.projectStructure
                        + "\n</project_structure>";
    }

    if (!data.currentFilePath.isEmpty()) {
        systemPrompt += "\n\n<current_file>\n"
                        + data.currentFilePath
                        + "\n</current_file>";
    }

    QJsonObject request{
        {"model", modelInfo.modelId},
        {"system", systemPrompt},
        {"max_tokens", data.maxTokens},
        {"messages", conversationHistory}
    };

    if (!tools.isEmpty())
        request["tools"] = tools;

    return QJsonDocument(request).toJson();
}

QList<ToolCall> ClaudeApiAdapter::parseToolCalls(const QByteArray &response)
{
    QList<ToolCall> toolCalls;

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(response, &parseError);

    if (parseError.error != QJsonParseError::NoError)
        return toolCalls;

    QJsonObject root = doc.object();

    if (root.contains("error"))
        return toolCalls;

    // Parse content array for tool_use blocks
    const QJsonArray content = root.value("content").toArray();
    for (const QJsonValue &value : content) {
        QJsonObject contentObj = value.toObject();
        QString contentType = contentObj.value("type").toString();

        if (contentType == "tool_use") {
            ToolCall call;
            call.id = contentObj.value("id").toString();
            call.toolName = contentObj.value("name").toString();
            call.arguments = contentObj.value("input").toObject();

            // Extract server name from prefixed tool name (server__tool)
            if (call.toolName.contains("__")) {
                QStringList parts = call.toolName.split("__");
                if (parts.size() == 2) {
                    call.serverName = parts.first();
                    call.toolName = parts.last();
                }
            }

            toolCalls.append(call);
        }
    }

    return toolCalls;
}

bool ClaudeApiAdapter::isResponseComplete(const QByteArray &response) const
{
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(response, &parseError);

    if (parseError.error != QJsonParseError::NoError)
        return true; // Assume complete on parse error

    QJsonObject root = doc.object();

    // Check stop_reason
    QString stopReason = root.value("stop_reason").toString();

    // "end_turn" means Claude is done
    // "tool_use" means Claude wants to use tools
    return stopReason == "end_turn";
}

AiResponse ClaudeApiAdapter::interpretResponse(const QByteArray &response)
{
    using Error = AiResponse::Error;

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(response, &parseError);

    if (parseError.error != QJsonParseError::NoError)
        return AiResponse(Error::JsonParseError);

    QJsonObject root = doc.object();

    if (root.contains("error")) {
        QString errorMsg = root.value("error").toObject().value("message").toString();
        return AiResponse::requestError(errorMsg);
    }

    // Extract text content
    QString contentStr = extractTextFromContent(root.value("content").toArray());

    // Parse for QML code blocks
    // This logic should match your existing AiApiUtils::aiResponseFromContent()
    AiResponse aiResponse;

    // Extract QML from code blocks
    static QRegularExpression qmlCodeRegex(
        R"(```(?:qml)?\s*\n(.*?)\n```)",
        QRegularExpression::DotMatchesEverythingOption);

    QRegularExpressionMatch match = qmlCodeRegex.match(contentStr);
    if (match.hasMatch())
        aiResponse.setQml(match.captured(1).trimmed());

    // Build a clean text for display in the chat UI:
    // remove QML code fences, XML tags (e.g. <selected_ids>), and collapse
    // excess blank lines left behind so the message reads naturally.
    QString displayText = contentStr;
    displayText.remove(qmlCodeRegex);
    static QRegularExpression xmlTagRegex(R"(<[^>]+>.*?</[^>]+>)",
                                          QRegularExpression::DotMatchesEverythingOption);
    displayText.remove(xmlTagRegex);
    // Collapse 3+ consecutive newlines down to two (one blank line max)
    static QRegularExpression excessNewlines(R"(\n{3,})");
    displayText.replace(excessNewlines, "\n\n");
    aiResponse.setText(displayText.trimmed());

    return aiResponse;
}

QJsonObject ClaudeApiAdapter::buildAssistantTurn(const QByteArray &response)
{
    QJsonArray content = extractContentArray(response);

    return QJsonObject{
        {"role", "assistant"},
        {"content", content}
    };
}

QJsonObject ClaudeApiAdapter::buildToolResultsTurn(const QList<ToolResult> &results)
{
    QJsonArray content;

    for (const ToolResult &result : results) {
        QJsonObject toolResult{
            {"type", "tool_result"},
            {"tool_use_id", result.toolCallId}
        };

        if (result.success) {
            toolResult["content"] = result.content;
        } else {
            toolResult["is_error"] = true;
            toolResult["content"] = result.error;
        }

        content.append(toolResult);
    }

    return QJsonObject{
        {"role", "user"},
        {"content", content}
    };
}

QString ClaudeApiAdapter::extractText(const QByteArray &response) const
{
    return extractTextFromContent(extractContentArray(response));
}

bool ClaudeApiAdapter::accepts(const QUrl &url) const
{
    QString strUrl = url.toString();
    return strUrl.compare("claude", Qt::CaseInsensitive) == 0
        || strUrl.contains("anthropic", Qt::CaseInsensitive);
}

QJsonArray ClaudeApiAdapter::extractContentArray(const QByteArray &response) const
{
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(response, &parseError);

    if (parseError.error != QJsonParseError::NoError)
        return QJsonArray();

    return doc.object().value("content").toArray();
}

QString ClaudeApiAdapter::extractTextFromContent(const QJsonArray &content) const
{
    QString text;

    for (const QJsonValue &value : content) {
        QJsonObject contentObj = value.toObject();
        QString contentType = contentObj.value("type").toString();

        if (contentType == "text")
            text += contentObj.value("text").toString();

        // Ignore tool_use blocks - they're handled by parseToolCalls
    }

    return text;
}

} // namespace QmlDesigner
