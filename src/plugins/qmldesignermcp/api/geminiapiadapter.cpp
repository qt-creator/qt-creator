// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "geminiapiadapter.h"

#include <agenticrequestmanager.h>
#include <aiassistantutils.h>
#include <aiproviderconfig.h>

#include <QFileInfo>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>

namespace QmlDesigner {

void GeminiApiAdapter::setRequestHeader(QNetworkRequest *request, const AiModelInfo &modelInfo)
{
    request->setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request->setRawHeader("x-goog-api-key", modelInfo.apiKey.toUtf8());
}

QByteArray GeminiApiAdapter::createRequest(
    const RequestData &data,
    const AiModelInfo &modelInfo,
    const QList<ToolEntry> &tools,
    const QList<ConversationMessage> &history)
{
    Q_UNUSED(modelInfo)

    QJsonObject root;
    root["contents"] = formatHistory(history);

    root["system_instruction"] = QJsonObject{
        {"parts", QJsonArray{ QJsonObject{{"text", data.instructions}} }}
    };

    QJsonArray formattedTools = formatTools(tools, true);
    if (!formattedTools.isEmpty())
        root["tools"] = formattedTools;

    // Add optional generation config
    QJsonObject generationConfig;
    generationConfig["temperature"] = 0.7;
    root["generationConfig"] = generationConfig;

    return QJsonDocument(root).toJson();
}

QList<ToolCall> GeminiApiAdapter::parseToolCalls(const QByteArray &response)
{
    QList<ToolCall> toolCalls;
    const QJsonArray parts = extractPartsArray(response);

    for (const QJsonValue &partValue : parts) {
        QJsonObject partObj = partValue.toObject();
        if (partObj.contains("functionCall")) {
            QJsonObject callObj = partObj.value("functionCall").toObject();

            ToolCall call;
            QString name = callObj.value("name").toString();

            if (name.contains("__")) {
                const QStringList parts = name.split("__");
                call.serverName = parts.first();
                name = parts.last();
            }

            call.toolName = name;
            call.id = name; // Gemini uses name instead of id
            call.arguments = callObj.value("args").toObject();

            toolCalls.append(call);
        }
    }

    return toolCalls;
}

bool GeminiApiAdapter::isResponseComplete(const QByteArray &response) const
{
    QJsonDocument doc = QJsonDocument::fromJson(response);
    if (doc.isNull())
        return true;

    const QJsonObject root = doc.object();

    if (root.contains("error"))
        return true;

    const QJsonArray candidates = root.value("candidates").toArray();
    if (candidates.isEmpty())
        return true;

    const QJsonObject candidate = candidates.first().toObject();
    const QString finishReason = candidate.value("finishReason").toString();

    // stream is likely still active
    if (finishReason.isEmpty())
        return false;

    // In Gemini, a function call has a finishReason of "STOP", so we must look at the actual payload.
    const QJsonObject content = candidate.value("content").toObject();
    const QJsonArray parts = content.value("parts").toArray();

    for (const QJsonValue &partValue : parts) {
        QJsonObject part = partValue.toObject();
        if (part.contains("functionCall"))
            return false;
    }

    // Standard terminal reasons: STOP, MAX_TOKENS, SAFETY, etc.
    return true;
}

QString GeminiApiAdapter::extractApiError(const QByteArray &response) const
{
    QJsonDocument doc = QJsonDocument::fromJson(response);
    if (doc.isNull())
        return QString("Malformed JSON response");

    const QJsonObject root = doc.object();

    // Top-level error object
    if (root.contains("error")) {
        const QJsonObject err = root.value("error").toObject();
        const int code = err.value("code").toInt();
        const QString message = err.value("message").toString();
        const QString status = err.value("status").toString();
        const QString detail = status.isEmpty() ? message
                                                : QString("%1: %2").arg(status, message);
        return code > 0 ? QString("[%1] %2").arg(code).arg(detail) : detail;
    }

    const QJsonArray candidates = root.value("candidates").toArray();

    // promptFeedback blockReason (e.g. SAFETY) comes without candidates
    if (candidates.isEmpty()) {
        const QString blockReason = root.value("promptFeedback")
        .toObject()
            .value("blockReason")
            .toString();
        if (!blockReason.isEmpty())
            return QString("Request blocked: %1").arg(blockReason);
        return {};
    }

    const QJsonObject candidate = candidates.first().toObject();
    const QString finishReason = candidate.value("finishReason").toString();

    if (finishReason == "MAX_TOKENS")
        return "The maximum number of tokens as specified in the request was reached.";

    if (finishReason == "SAFETY")
        return "The response content was flagged for safety reasons.";

    if (finishReason == "RECITATION")
        return "The response content was flagged for recitation reasons.";

    if (finishReason == "LANGUAGE")
        return "The response content was flagged for using an unsupported language.";

    if (finishReason == "BLOCKLIST")
        return "The response stopped because the content contains forbidden terms.";

    if (finishReason == "PROHIBITED_CONTENT")
        return "The response stopped for potentially containing prohibited content.";

    if (finishReason == "SPII") {
        return "The response stopped because the content potentially contains Sensitive "
               "Personally Identifiable Information (SPII).";
    }

    if (finishReason == "MALFORMED_FUNCTION_CALL")
        return "The function call generated by the model is invalid.";

    if (finishReason == "UNEXPECTED_TOOL_CALL")
        return "Model generated a tool call but no tools were enabled in the request.";

    if (finishReason == "TOO_MANY_TOOL_CALLS")
        return "Model called too many tools consecutively, thus the system exited execution.";

    if (finishReason == "MISSING_THOUGHT_SIGNATURE")
        return "Request has at least one thought signature missing.";

    if (finishReason == "MALFORMED_RESPONSE")
        return "Finished due to malformed response.";

    if (!finishReason.isEmpty() && finishReason != "STOP")
        return QString("Unexpected finish reason: %1").arg(finishReason);

    return {};
}

QList<MessageBlock> GeminiApiAdapter::parseResponse(const QByteArray &response)
{
    QList<MessageBlock> blocks;

    const QJsonArray parts = extractPartsArray(response);
    for (const QJsonValue &partValue : parts) {
        QJsonObject partObj = partValue.toObject();

        if (partObj.contains("text")) {
            MessageBlock block;
            block.type = MessageBlock::Type::Text;
            block.text = partObj.value("text").toString();
            blocks.append(block);

        } else if (partObj.contains("functionCall")) {
            QJsonObject callObj = partObj.value("functionCall").toObject();
            QString name = callObj.value("name").toString();

            MessageBlock block;
            block.type = MessageBlock::Type::ToolUse;
            block.arguments = callObj.value("args").toObject();
            block.auxiliaryData = partObj.value("thoughtSignature").toString();

            if (name.contains("__")) {
                const QStringList parts = name.split("__");
                block.serverName = parts.first();
                block.toolName = parts.last();
            } else {
                block.toolName = name;
            }

            block.id = block.toolName; // Gemini uses name as id
            blocks.append(block);
        }
    }

    return blocks;
}

QJsonArray GeminiApiAdapter::formatHistory(const QList<ConversationMessage> &messages) const
{
    QJsonArray history;

    for (const ConversationMessage &message : messages) {
        switch (message.role) {
        case ConversationMessage::Role::User: {
            QJsonArray parts;
            for (const MessageBlock &block : message.blocks) {
                if (block.type == MessageBlock::Type::Text) {
                    parts.append(QJsonObject{{"text", block.text}});
                } else if (block.type == MessageBlock::Type::Image) {
                    const QString filePath = block.url.toLocalFile();
                    const QByteArray fmt = QFileInfo(filePath).suffix().toLatin1();
                    parts.append(QJsonObject{
                        {"inline_data", QJsonObject{
                                            {"mime_type", QString("image/%1").arg(QString::fromLatin1(fmt))},
                                            {"data", AiAssistantUtils::toBase64Image(QImage(filePath), fmt)},
                                        }}
                    });
                }
            }
            history.append(QJsonObject{{"role", "user"}, {"parts", parts}});
            break;
        }

        case ConversationMessage::Role::Assistant: {
            QJsonArray parts;
            for (const MessageBlock &block : message.blocks) {
                if (block.type == MessageBlock::Type::Text) {
                    parts.append(QJsonObject{{"text", block.text}});
                } else if (block.type == MessageBlock::Type::ToolUse) {
                    const QString wireName = block.serverName.isEmpty()
                    ? block.toolName
                    : QString("%1__%2").arg(block.serverName,
                                            block.toolName);
                    parts.append(QJsonObject{
                        {"functionCall", QJsonObject{{"name", wireName}, {"args", block.arguments}}},
                        {"thoughtSignature", block.auxiliaryData }
                    });
                }
            }
            history.append(QJsonObject{{"role", "model"}, {"parts", parts}});
            break;
        }

        case ConversationMessage::Role::ToolResults: {
            // Gemini expects function responses as a user-role turn
            QJsonArray parts;
            for (const MessageBlock &block : message.blocks) {
                if (block.type != MessageBlock::Type::ToolResult)
                    continue;

                parts.append(QJsonObject{
                    {"functionResponse", QJsonObject{
                                             {"name", block.toolName},
                                             {"response", QJsonObject{
                                                              {"output", block.success ? block.content
                                                                                       : block.error}
                                                          }}
                                         }}
                });
            }
            history.append(QJsonObject{{"role", "user"}, {"parts", parts}});
            break;
        }
        }
    }

    return history;
}

namespace {

QJsonObject sanitizeSchema(const QJsonObject &schema)
{
    // Gemini's function declarations don't support $schema or additionalProperties
    static const QStringList blockedKeys = {"$schema", "additionalProperties"};

    QJsonObject result;
    for (auto it = schema.constBegin(); it != schema.constEnd(); ++it) {
        if (blockedKeys.contains(it.key()))
            continue;

        // Recurse into nested objects
        if (it.value().isObject()) {
            result[it.key()] = sanitizeSchema(it.value().toObject());
        } else if (it.value().isArray()) {
            QJsonArray arr;
            for (const QJsonValue &v : it.value().toArray())
                arr.append(v.isObject() ? sanitizeSchema(v.toObject()) : v);
            result[it.key()] = arr;
        } else {
            result[it.key()] = it.value();
        }
    }
    return result;
}

} // namespace

QJsonArray GeminiApiAdapter::formatTools(const QList<ToolEntry> &tools, bool prefixWithServer) const
{
    QJsonArray declarations;
    for (const auto &entry : tools) {
        QString toolName = prefixWithServer ? QString("%1__%2").arg(entry.serverName, entry.name) : entry.name;

        declarations.append(QJsonObject{
            {"name", toolName},
            {"description", entry.description},
            {"parameters", sanitizeSchema(entry.inputSchema)}
        });
    }

    return QJsonArray{ QJsonObject{{"functionDeclarations", declarations}} };
}

bool GeminiApiAdapter::accepts(const QString &url) const
{
    return url.contains("googleapis.com") || url.contains("gemini");
}

QJsonArray GeminiApiAdapter::extractPartsArray(const QByteArray &response) const
{
    QJsonDocument doc = QJsonDocument::fromJson(response);
    QJsonArray candidates = doc.object().value("candidates").toArray();
    if (candidates.isEmpty())
        return {};

    return candidates.first().toObject().value("content").toObject().value("parts").toArray();
}

QUrl GeminiApiAdapter::resolveUrl(const QString &baseUrl, const AiModelInfo &modelInfo) const
{
    return QUrl(QString(baseUrl).replace("{modelId}", modelInfo.modelId));
}

} // namespace QmlDesigner
