// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "claudemessagesapi.h"

#include "aiapiutils.h"
#include "airesponse.h"

#include <utils/filepath.h>

#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>

namespace QmlDesigner {

namespace {

QJsonObject toJsonImage(const Utils::FilePath &imagePath)
{
    QImage image(imagePath.toFSPathString());
    const QByteArray imageFormat = imagePath.suffix().toLatin1();
    return {
        {"type", "image"},
        {
            "source",
            QJsonObject{
                {"type", "base64"},
                {"media_type", QString("image/%1").arg(imageFormat)},
                {"data", AiApiUtils::toBase64Image(image, imageFormat)},
            },
        },
    };
}

QJsonArray getUserJson(const QUrl &imageUrl, const QString &currentQml, const QString &prompt)
{
    using namespace Qt::StringLiterals;
    using Utils::FilePath;

    QJsonArray jsonContent;

    jsonContent << QJsonObject{
        {"type", "text"},
        {"text", "Current Qml:\n```qml\n%1\n```"_L1.arg(currentQml)},
    };

    jsonContent << QJsonObject{
        {"type", "text"},
        {"text", "Request: %1"_L1.arg(prompt)},
    };

    if (!imageUrl.isEmpty()) {
        FilePath imagePath = FilePath::fromUrl(imageUrl);
        if (imagePath.exists())
            jsonContent << toJsonImage(imagePath);
    }

    return jsonContent;
}

} // namespace

void ClaudeMessagesApi::setRequestHeader(
    QNetworkRequest *networkRequest, const AiModelInfo &modelInfo)
{
    networkRequest->setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    networkRequest->setRawHeader("x-api-key", modelInfo.apiKey.toUtf8());
    networkRequest->setRawHeader("anthropic-version", "2023-06-01");
}

QByteArray ClaudeMessagesApi::createContent(const Data &data, const AiModelInfo &modelInfo)
{
    QJsonArray userJson = getUserJson(data.attachedImage, data.qml, data.userPrompt);

    QJsonObject json = {
        {"model", modelInfo.modelId},
        {"system", data.manifest},
        {"max_tokens", 4096}, // TODO: Add a user setting for this parameter
        {
            "messages",
            QJsonArray{
                QJsonObject{
                    {"role", "user"},
                    {"content", userJson},
                },
            },
        },
    };

    return QJsonDocument(json).toJson();
}

AiResponse ClaudeMessagesApi::interpretResponse(const QByteArray &jsonData)
{
    using Error = AiResponse::Error;

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError)
        return Error::JsonParseError;

    QJsonObject root = doc.object();

    if (root.contains("error"))
        return AiResponse::requestError(root.value("error").toObject().value("message").toString());

    QString contentStr;
    if (root.contains("content")) {
        const QJsonArray content = root["content"].toArray();
        for (const QJsonValue &value : content) {
            QJsonObject contentObj = value.toObject();
            QString contentType = contentObj["type"].toString();

            if (contentType == "text") {
                QString text = contentObj["text"].toString();
                contentStr.append(text);
            }
        }
    }

    return AiApiUtils::aiResponseFromContent(contentStr);
}

bool ClaudeMessagesApi::accepts(const AiModelInfo &info)
{
    if (!info.isValid())
        return false;

    const QString url = info.url.toString();
    return url.contains("anthropic", Qt::CaseInsensitive)
           && url.contains("/messages", Qt::CaseInsensitive);
}

} // namespace QmlDesigner
