// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "claudeapiadapter.h"

#include "agenticrequestmanager.h"
#include "aiassistantutils.h"
#include "airesponse.h"

#include <QFileInfo>
#include <QImage>
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
    const QList<ToolEntry> &tools,
    const QList<ConversationTurn> &history)
{
    QJsonObject request{
        {"model", modelInfo.modelId},
        {"system", data.instructions},
        {"max_tokens", data.maxTokens},
        {"messages", formatHistory(history)}
    };

    QJsonArray formattedTools = formatTools(tools, true);
    if (!formattedTools.isEmpty())
        request["tools"] = formattedTools;

    return QJsonDocument(request).toJson();
}

QString ClaudeApiAdapter::id() const
{
    return "claude";
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

    return AiResponse(contentStr);
}

QJsonArray ClaudeApiAdapter::buildUserMessage(const QString &text, const QUrl &imageUrl)
{
    QJsonArray content;
    content.append(QJsonObject{{"type", "text"}, {"text", text}});

    if (!imageUrl.isEmpty()) {
        const QString filePath = imageUrl.toLocalFile();
        const QByteArray fmt = QFileInfo(filePath).suffix().toLatin1();
        content.append(QJsonObject{
            {"type", "image"},
            {"source", QJsonObject{
                {"type", "base64"},
                {"media_type", QString("image/%1").arg(QString::fromLatin1(fmt))},
                {"data", AiAssistantUtils::toBase64Image(QImage(filePath), fmt)},
            }},
        });
    }

    return content;
}

QJsonArray ClaudeApiAdapter::buildAssistantTurn(const QByteArray &response)
{
    return QJsonArray{QJsonObject{
        {"role", "assistant"},
        {"content", extractContentArray(response)},
    }};
}

QJsonArray ClaudeApiAdapter::buildToolResultsTurn(const QList<ToolResult> &results)
{
    QJsonArray content;

    for (const ToolResult &result : results) {
        QJsonObject toolResult{
            {"type", "tool_result"},
            {"tool_use_id", result.toolCallId},
            {"content", result.success ? result.content : result.error}
        };

        if (!result.success)
            toolResult["is_error"] = true;

        content.append(toolResult);
    }

    return content;
}

QJsonArray ClaudeApiAdapter::formatHistory(const QList<ConversationTurn> &turns) const
{
    QJsonArray history;
    for (const ConversationTurn &turn : turns) {
        if (turn.type == ConversationTurn::ToolResults) {
            // tool results turn: content is a flat array of tool_result items,
            // needs to be wrapped in a user message
            history.append(QJsonObject{{"role", "user"}, {"content", turn.content}});
        } else {
            // user and assistant turns: content is [{role, content:[...]}], take first
            history.append(turn.content.first());
        }
    }
    return history;
}

QJsonArray ClaudeApiAdapter::formatTools(const QList<ToolEntry> &tools, bool prefixWithServer) const
{
    QJsonArray result;
    for (const ToolEntry &entry : tools) {
        const QString toolName = prefixWithServer
                        ? QString("%1__%2").arg(entry.serverName, entry.name) : entry.name;
        result.append(QJsonObject{
            {"name",         toolName},
            {"description",  entry.description},
            {"input_schema", entry.inputSchema}
        });
    }
    return result;
}

QString ClaudeApiAdapter::extractText(const QByteArray &response) const
{
    return extractTextFromContent(extractContentArray(response));
}

bool ClaudeApiAdapter::accepts(const QString &url) const
{
    return url.compare("claude", Qt::CaseInsensitive) == 0
        || url.contains("anthropic", Qt::CaseInsensitive);
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
