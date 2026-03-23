// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "openaiapiresponseadapter.h"

#include "agenticrequestmanager.h"
#include "aiassistantutils.h"
#include "aiproviderconfig.h"
#include "airesponse.h"

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
    const QJsonArray &tools,
    const QJsonArray &conversationHistory)
{
    QString systemText = data.instructions;

    if (!data.projectStructure.isEmpty()) {
        systemText += "\n\n<project_structure>\n"
                      + data.projectStructure
                      + "\n</project_structure>";
    }

    if (!data.currentFilePath.isEmpty()) {
        systemText += "\n\n<current_file>\n"
                      + data.currentFilePath
                      + "\n</current_file>";
    }

    QJsonObject requestObj{
        {"model", modelInfo.modelId},
        {"instructions", systemText},
        {"max_output_tokens", data.maxTokens},
        {"input", conversationHistory}
    };

    if (!m_previousResponseId.isEmpty())
        requestObj["previous_response_id"] = m_previousResponseId;

    if (!tools.isEmpty())
        requestObj["tools"] = tools;

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

    // The Responses API returns an "output" array. Each item has a "type".
    // Tool invocations appear as items of type "function_call".
    const QJsonArray output = root.value("output").toArray();
    for (const QJsonValue &value : output) {
        QJsonObject item = value.toObject();
        if (item.value("type").toString() != "function_call")
            continue;

        ToolCall call {
            .id       = item.value("call_id").toString(),
            .toolName = item.value("name").toString()
        };

        // Arguments arrive as a JSON-encoded string; parse it back to an object.
        QString argsStr = item.value("arguments").toString();
        QJsonDocument argsDoc = QJsonDocument::fromJson(argsStr.toUtf8());
        if (argsDoc.isObject())
            call.arguments = argsDoc.object();

        // Extract server name from prefixed tool name (server__tool).
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

    // Top-level "status" field: "completed" | "requires_action" | "failed" | ...
    // "requires_action" signals that tool calls are pending.
    QString status = root.value("status").toString();
    return status != "requires_action";
}

AiResponse OpenAiResponseApiAdapter::interpretResponse(const QByteArray &response)
{
    using Error = AiResponse::Error;

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(response, &parseError);
    if (parseError.error != QJsonParseError::NoError)
        return AiResponse(Error::JsonParseError);

    QJsonObject root = doc.object();

    if (root.contains("error") && !root.value("error").isNull()) {
        QString errorMsg = root.value("error").toObject().value("message").toString();
        return AiResponse::requestError(errorMsg);
    }

    m_previousResponseId = doc.object().value("id").toString();

    QString contentStr = extractTextFromOutput(root.value("output").toArray());

    return AiResponse(contentStr);
}

QJsonArray OpenAiResponseApiAdapter::buildUserMessage(const QString &text, const QUrl &imageUrl)
{
    QJsonArray content;
    content.append(QJsonObject{{"type", "input_text"}, {"text", text}});

    if (!imageUrl.isEmpty()) {
        const QString filePath = imageUrl.toLocalFile();
        const QByteArray fmt = QFileInfo(filePath).suffix().toLatin1();
        const QString b64 = AiAssistantUtils::toBase64Image(QImage(filePath), fmt);
        content.append(QJsonObject{
            {"type", "input_image"},
            {"image_url", QString("data:image/%1;base64,%2").arg(QString::fromLatin1(fmt), b64)},
        });
    }

    return content;
}

QJsonArray OpenAiResponseApiAdapter::buildAssistantTurn(const QByteArray &response)
{
    // Capture the response ID for the next iteration
    QJsonDocument doc = QJsonDocument::fromJson(response);
    m_previousResponseId = doc.object().value("id").toString();

    // No history items to return — state is managed server-side
    return {};
}

QJsonArray OpenAiResponseApiAdapter::buildToolResultsTurn(const QList<ToolResult> &results)
{
    // Tool results are sent as the input for the next iteration
    QJsonArray items;
    for (const ToolResult &result : results) {
        items.append(QJsonObject{
            {"type", "function_call_output"},
            {"call_id", result.toolCallId},
            {"output", result.success
                           ? result.content
                           : QString(R"({"error": "%1"})").arg(result.error)},
        });
    }
    return items;
}

QJsonArray OpenAiResponseApiAdapter::formatHistory(const QList<ConversationTurn> &turns) const
{
    if (turns.isEmpty())
        return {};

    const ConversationTurn &last = turns.last();

    // Collect ALL consecutive tool results from the end
    if (last.type == ConversationTurn::ToolResults) {
        // Find the first ToolResults turn in the trailing block
        int start = turns.size() - 1;
        while (start > 0 && turns[start - 1].type == ConversationTurn::ToolResults)
            --start;

        QJsonArray input;
        for (int i = start; i < turns.size(); ++i) {
            for (const QJsonValue &item : std::as_const(turns[i]).content)
                input.append(item);
        }

        return input;
    }

    // With previous_response_id, only the latest turn needs to be sent
    return QJsonArray{QJsonObject{
        {"type", "message"},
        {"role", last.role},
        {"content", last.content},
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

bool OpenAiResponseApiAdapter::accepts(const QUrl &url) const
{
    QString strUrl = url.toString();
    return strUrl.contains("openai.com", Qt::CaseInsensitive)
        || strUrl.contains("responses", Qt::CaseInsensitive);
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
