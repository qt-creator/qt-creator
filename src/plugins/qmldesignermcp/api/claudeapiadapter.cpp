// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "claudeapiadapter.h"

#include "agenticrequestmanager.h"
#include "aiassistantutils.h"

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
    const QList<ConversationMessage> &history)
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
        return true; // Stop at malformed response

    const QJsonObject root = doc.object();

    // API-level error block, caller reads extractApiError()
    if (root.contains("error"))
        return true;

    const QString stopReason = root.value("stop_reason").toString();

    // model wants to call a tool
    if (stopReason == "tool_use")
        return false;

    // normal completion signal
    if (stopReason == "end_turn")
        return true;

    // Terminate on other stop reasons (like max_tokens, stop_sequence, content_filtered, etc)
    if (!stopReason.isEmpty())
        return true;

    // No stop_reason yet, treat as incomplete
    return false;
}

QString ClaudeApiAdapter::extractApiError(const QByteArray &response) const
{
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(response, &parseError);

    if (parseError.error != QJsonParseError::NoError)
        return QString("Malformed JSON response: %1").arg(parseError.errorString());

    const QJsonObject root = doc.object();

    // Explicit error object returned by the API
    if (root.contains("error")) {
        const QJsonObject err = root.value("error").toObject();
        const QString type    = err.value("type").toString();
        const QString message = err.value("message").toString();
        return type.isEmpty() ? message : QString("%1: %2").arg(type, message);
    }

    // Non-successful stop reasons that are not tool_use / end_turn
    const QString stopReason = root.value("stop_reason").toString();
    if (stopReason == "max_tokens")
        return "Claude stopped because it reached the max tokens limit.";

    if (stopReason == "stop_sequence")
        return "Claude encountered one of your custom stop sequences.";

    if (stopReason == "refusal")
        return "Claude refused to generate a response due to safety concerns.";

    if (stopReason == "model_context_window_exceeded")
        return "Claude stopped because it reached the model's context window limit.";

    if (!stopReason.isEmpty() && stopReason != "end_turn" && stopReason != "tool_use")
        return QString("Unexpected stop reason: %1").arg(stopReason);

    return {};
}

QList<MessageBlock> ClaudeApiAdapter::parseResponse(const QByteArray &response)
{
    QList<MessageBlock> blocks;

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(response, &parseError);
    if (parseError.error != QJsonParseError::NoError)
        return blocks;

    const QJsonArray content = doc.object().value("content").toArray();
    for (const QJsonValue &value : content) {
        QJsonObject obj = value.toObject();
        const QString type = obj.value("type").toString();

        if (type == "text") {
            MessageBlock block;
            block.type = MessageBlock::Type::Text;
            block.text = obj.value("text").toString();
            blocks.append(block);

        } else if (type == "tool_use") {
            MessageBlock block;
            block.type = MessageBlock::Type::ToolUse;
            block.id = obj.value("id").toString();
            block.toolName = obj.value("name").toString();
            block.arguments = obj.value("input").toObject();

            if (block.toolName.contains("__")) {
                QStringList parts = block.toolName.split("__");
                if (parts.size() == 2) {
                    block.serverName = parts.first();
                    block.toolName = parts.last();
                }
            }

            blocks.append(block);
        }
    }

    return blocks;
}

QJsonArray ClaudeApiAdapter::formatHistory(const QList<ConversationMessage> &messages) const
{
    QJsonArray history;

    for (const ConversationMessage &message : messages) {
        switch (message.role) {
        case ConversationMessage::Role::User: {
            QJsonArray content;
            for (const MessageBlock &block : message.blocks) {
                if (block.type == MessageBlock::Type::Text) {
                    content.append(QJsonObject{{"type", "text"}, {"text", block.text}});
                } else if (block.type == MessageBlock::Type::Image) {
                    const QString filePath = block.url.toLocalFile();
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
            }
            history.append(QJsonObject{{"role", "user"}, {"content", content}});
            break;
        }

        case ConversationMessage::Role::Assistant: {
            QJsonArray content;
            for (const MessageBlock &block : message.blocks) {
                if (block.type == MessageBlock::Type::Text) {
                    content.append(QJsonObject{{"type", "text"}, {"text", block.text}});
                } else if (block.type == MessageBlock::Type::ToolUse) {
                    // Reconstruct prefixed name for the wire format
                    const QString wireName = block.serverName.isEmpty()
                                                 ? block.toolName
                                                 : QString("%1__%2").arg(block.serverName,
                                                                         block.toolName);
                    content.append(QJsonObject{
                                       {"type", "tool_use"},
                                       {"id", block.id},
                                       {"name", wireName},
                                       {"input", block.arguments},
                                   });
                }
            }
            history.append(QJsonObject{{"role", "assistant"}, {"content", content}});
            break;
        }

        case ConversationMessage::Role::ToolResults: {
            // Claude expects tool results as a user-role message
            QJsonArray content;
            for (const MessageBlock &block : message.blocks) {
                if (block.type != MessageBlock::Type::ToolResult)
                    continue;

                QJsonObject toolResult{
                    {"type", "tool_result"},
                    {"tool_use_id", block.toolCallId},
                    {"content", block.success ? block.content : block.error},
                };
                if (!block.success)
                    toolResult["is_error"] = true;

                content.append(toolResult);
            }
            history.append(QJsonObject{{"role", "user"}, {"content", content}});
            break;
        }
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

bool ClaudeApiAdapter::accepts(const QString &url) const
{
    return url.compare("claude", Qt::CaseInsensitive) == 0
        || url.contains("anthropic", Qt::CaseInsensitive);
}

} // namespace QmlDesigner
