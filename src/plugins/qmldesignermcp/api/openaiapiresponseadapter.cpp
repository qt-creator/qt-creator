// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "openaiapiresponseadapter.h"

#include "agenticrequestmanager.h"
#include "aiassistantutils.h"
#include "aiproviderconfig.h"

#include <QFileInfo>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>

namespace QmlDesigner {

OpenAiResponseApiAdapter::OpenAiResponseApiAdapter()
{}

void OpenAiResponseApiAdapter::setRequestHeader(QNetworkRequest *request,
                                                const AiModelInfo &modelInfo)
{
    request->setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request->setRawHeader("Authorization", QByteArray("Bearer ") + modelInfo.apiKey.toUtf8());
}

QByteArray OpenAiResponseApiAdapter::createRequest(
    const RequestData &data,
    const AiModelInfo &modelInfo,
    const QList<ToolEntry> &tools,
    const QList<ConversationMessage> &history)
{
    QJsonObject requestObj{
        {"model", modelInfo.modelId},
        {"instructions", data.instructions},
        {"max_output_tokens", data.maxTokens},
        {"input", formatHistory(history)}
    };

    if (!m_previousResponseId.isEmpty())
        requestObj["previous_response_id"] = m_previousResponseId;

    QJsonArray formattedTools = formatTools(tools, true);
    if (!formattedTools.isEmpty())
        requestObj["tools"] = formattedTools;

    return QJsonDocument(requestObj).toJson();
}

QString OpenAiResponseApiAdapter::id() const
{
    return "openai_responses";
}

QList<ToolCall> OpenAiResponseApiAdapter::parseToolCalls(const QByteArray &response)
{
    QList<ToolCall> toolCalls;

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(response, &parseError);
    if (parseError.error != QJsonParseError::NoError)
        return {};

    QJsonObject root = doc.object();
    if (root.value("error").isObject())
        return {};

    const QJsonArray output = root.value("output").toArray();
    for (const QJsonValue &value : output) {
        QJsonObject item = value.toObject();
        if (item.value("type").toString() != "function_call")
            continue;

        ToolCall call {
            .id       = item.value("call_id").toString(),
            .toolName = item.value("name").toString()
        };

        QString argsStr = item.value("arguments").toString();
        QJsonDocument argsDoc = QJsonDocument::fromJson(argsStr.toUtf8());
        if (argsDoc.isObject())
            call.arguments = argsDoc.object();

        if (call.toolName.contains("__")) {
            QStringList parts = call.toolName.split("__");
            if (parts.size() == 2) {
                call.serverName = parts.first();
                call.toolName   = parts.last();
            }
        }

        toolCalls.append(call);
    }

    return toolCalls;
}

bool OpenAiResponseApiAdapter::isResponseComplete(const QByteArray &response) const
{
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(response, &parseError);
    if (parseError.error != QJsonParseError::NoError)
        return true; // Treat unparseable responses as complete.

    QJsonObject root = doc.object();

    const QJsonArray output = doc.object().value("output").toArray();
    for (const QJsonValue &value : output) {
        if (value.toObject().value("type").toString() == "function_call")
            return false;
    }
    return true;
}

QList<MessageBlock> OpenAiResponseApiAdapter::parseResponse(const QByteArray &response)
{
    QList<MessageBlock> blocks;

    QJsonDocument doc = QJsonDocument::fromJson(response);
    m_previousResponseId = doc.object().value("id").toString();

    const QJsonArray output = doc.object().value("output").toArray();
    for (const QJsonValue &value : output) {
        QJsonObject item = value.toObject();
        const QString type = item.value("type").toString();

        if (type == "message") {
            for (const QJsonValue &cv : item.value("content").toArray()) {
                QJsonObject c = cv.toObject();
                if (c.value("type").toString() == "output_text") {
                    MessageBlock block;
                    block.type = MessageBlock::Type::Text;
                    block.text = c.value("text").toString();
                    blocks.append(block);
                }
            }
        } else if (type == "function_call") {
            MessageBlock block;
            block.type = MessageBlock::Type::ToolUse;
            block.id = item.value("call_id").toString();
            block.toolName = item.value("name").toString();

            QJsonDocument argsDoc = QJsonDocument::fromJson(item.value("arguments").toString().toUtf8());
            if (argsDoc.isObject())
                block.arguments = argsDoc.object();

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

QJsonArray OpenAiResponseApiAdapter::formatHistory(const QList<ConversationMessage> &messages) const
{
    if (messages.isEmpty())
        return {};

    const ConversationMessage &last = messages.last();

    // With previous_response_id, the Responses API only needs the latest user
    // input or tool results — the rest is managed server-side.
    if (last.role == ConversationMessage::Role::ToolResults) {
        // Collect all consecutive trailing ToolResults turns
        int start = messages.size() - 1;
        while (start > 0 && messages[start - 1].role == ConversationMessage::Role::ToolResults)
            --start;

        QJsonArray input;
        for (int i = start; i < messages.size(); ++i) {
            for (const MessageBlock &block : messages[i].blocks) {
                if (block.type != MessageBlock::Type::ToolResult)
                    continue;

                input.append(QJsonObject{
                                 {"type", "function_call_output"},
                                 {"call_id", block.toolCallId},
                                 {"output", block.success
                                                ? block.content
                                                : QString(R"({"error": "%1"})").arg(block.error)},
                             });
            }
        }
        return input;
    }

    // Latest user message
    QJsonArray content;
    for (const MessageBlock &block : last.blocks) {
        if (block.type == MessageBlock::Type::Text) {
            content.append(QJsonObject{{"type", "input_text"}, {"text", block.text}});
        } else if (block.type == MessageBlock::Type::Image) {
            const QString filePath = block.url.toLocalFile();
            const QString fmt = QFileInfo(filePath).suffix();
            const QString b64 = AiAssistantUtils::toBase64Image(QImage(filePath), fmt.toLatin1());
            content.append(QJsonObject{
                               {"type", "input_image"},
                               {"image_url", QString("data:image/%1;base64,%2").arg(fmt, b64)},
                           });
        }
    }

    return QJsonArray{QJsonObject{
                         {"type", "message"},
                         {"role", "user"},
                         {"content", content},
                     }};
}

QJsonArray OpenAiResponseApiAdapter::formatTools(const QList<ToolEntry> &tools,
                                                 bool prefixWithServer) const
{
    QJsonArray result;
    for (const ToolEntry &entry : tools) {
        const QString toolName = prefixWithServer
                            ? QString("%1__%2").arg(entry.serverName, entry.name) : entry.name;
        result.append(QJsonObject{
            {"type",        "function"},
            {"name",        toolName},
            {"description", entry.description},
            {"parameters",  entry.inputSchema}
        });
    }
    return result;
}

QString OpenAiResponseApiAdapter::extractText(const QByteArray &response) const
{
    return extractTextFromOutput(extractOutputArray(response));
}

bool OpenAiResponseApiAdapter::accepts(const QString &url) const
{
    return url.contains("openai.com", Qt::CaseInsensitive)
        || url.contains("responses", Qt::CaseInsensitive);
}

void OpenAiResponseApiAdapter::clear()
{
    m_previousResponseId.clear();
}

QJsonArray OpenAiResponseApiAdapter::extractOutputArray(const QByteArray &response) const
{
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(response, &parseError);
    if (parseError.error != QJsonParseError::NoError)
        return QJsonArray();

    return doc.object().value("output").toArray();
}

QString OpenAiResponseApiAdapter::extractTextFromOutput(const QJsonArray &output) const
{
    QString text;

    for (const QJsonValue &value : output) {
        QJsonObject item = value.toObject();

        // Only message-type items carry readable text.
        if (item.value("type").toString() != "message")
            continue;

        const QJsonArray content = item.value("content").toArray();
        for (const QJsonValue &cv : content) {
            QJsonObject contentObj = cv.toObject();
            if (contentObj.value("type").toString() == "output_text")
                text += contentObj.value("text").toString();
        }
    }

    return text;
}

} // namespace QmlDesigner
