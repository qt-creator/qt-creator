// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "geminiapiadapter.h"

#include <agenticrequestmanager.h>
#include <aiassistantutils.h>
#include <aiproviderconfig.h>
#include <airesponse.h>

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
    const QList<ConversationTurn> &history)
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
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(response, &parseError);

    if (parseError.error != QJsonParseError::NoError)
        return true; // complete on parse error

    QJsonObject root = doc.object();
    QJsonArray candidates = root.value("candidates").toArray();

    if (candidates.isEmpty())
        return true;

    QJsonObject firstCandidate = candidates.first().toObject();

    QString finishReason = firstCandidate.value("finishReason").toString();

    return finishReason != "FUNCTION_CALL";
}

AiResponse GeminiApiAdapter::interpretResponse(const QByteArray &response)
{
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(response, &parseError);

    if (parseError.error != QJsonParseError::NoError)
        return AiResponse(AiResponse::Error::JsonParseError, parseError.errorString());

    QJsonObject root = doc.object();

    if (root.contains("error")) {
        QString errorMsg = root.value("error").toObject().value("message").toString();
        return AiResponse::requestError(errorMsg);
    }

    // Check for model-level blocks (safety, etc)
    QJsonArray candidates = root.value("candidates").toArray();
    if (candidates.isEmpty())
        return AiResponse(AiResponse::Error::EmptyResponse);

    QJsonObject firstCandidate = candidates.first().toObject();
    QString finishReason = firstCandidate.value("finishReason").toString();

    if (finishReason == "SAFETY" || finishReason == "RECITATION")
        return AiResponse(AiResponse::Error::RequestError);

    QString text = extractText(response).trimmed();
    if (text.isEmpty() && finishReason == "STOP")
        text = "Done.";

    return AiResponse(text);
}

QJsonArray GeminiApiAdapter::buildUserMessage(const QString &text, const QUrl &imageUrl)
{
    QJsonArray parts;
    parts.append(QJsonObject{{"text", text}});

    if (!imageUrl.isEmpty()) {
        const QString filePath = imageUrl.toLocalFile();
        const QByteArray fmt = QFileInfo(filePath).suffix().toLatin1();
        parts.append(QJsonObject{
            {"inline_data", QJsonObject{
                {"mime_type", QString("image/%1").arg(QString::fromLatin1(fmt))},
                {"data", AiAssistantUtils::toBase64Image(QImage(filePath), fmt)},
            }}
        });
    }

    return QJsonArray{ QJsonObject{
        {"role", "user"},
        {"parts", parts}
    }};
}

QJsonArray GeminiApiAdapter::buildAssistantTurn(const QByteArray &response)
{
    QJsonDocument doc = QJsonDocument::fromJson(response);
    const QJsonObject content = doc.object()
                                    .value("candidates").toArray()
                                    .first().toObject()
                                    .value("content").toObject();

    return QJsonArray{ QJsonObject{
        {"role", content.value("role").toString("model")},
        {"parts", content.value("parts").toArray()},
    }};
}

QJsonArray GeminiApiAdapter::buildToolResultsTurn(const QList<ToolResult> &results)
{
    QJsonArray parts;
    for (const auto &result : results) {
        parts.append(QJsonObject{
            {"functionResponse", QJsonObject{
                {"name", result.toolName},
                {"response", QJsonObject{{"output", result.success ? result.content : result.error}}}
            }}
        });
    }

    return QJsonArray{ QJsonObject{
        {"role", "user"},
        {"parts", parts}
    }};
}

QJsonArray GeminiApiAdapter::formatHistory(const QList<ConversationTurn> &turns) const
{
    QJsonArray history;
    for (const ConversationTurn &turn : turns)
        history.append(turn.content.first()); // turn.content is [{role, parts: [{text:...}]}]
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

QString GeminiApiAdapter::extractText(const QByteArray &response) const
{
    QString text;
    const QJsonArray parts = extractPartsArray(response);

    for (const QJsonValue &partValue : parts) {
        QJsonObject partObj = partValue.toObject();
        if (partObj.contains("text"))
            text += partObj.value("text").toString();
    }

    return text;
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
