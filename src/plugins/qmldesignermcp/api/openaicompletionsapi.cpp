// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "openaicompletionsapi.h"

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
    using namespace Qt::StringLiterals;
    QImage image(imagePath.toFSPathString());
    const QByteArray imageFormat = imagePath.suffix().toLatin1();
    return {
        {"type", "image_url"},
        {
            "image_url",
            QJsonObject{
                {"url",
                 "data:image/%1;base64,%2"_L1
                     .arg(imageFormat, AiApiUtils::toBase64Image(image, imageFormat))},
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

void OpenAiCompletionsApi::setRequestHeader(
    QNetworkRequest *networkRequest, const AiModelInfo &modelInfo)
{
    networkRequest->setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    networkRequest
        ->setRawHeader("Authorization", QByteArray("Bearer ").append(modelInfo.apiKey.toUtf8()));
}

QByteArray OpenAiCompletionsApi::createContent(const Data &data, const AiModelInfo &modelInfo)
{
    QJsonArray userJson = getUserJson(data.attachedImage, data.qml, data.userPrompt);

    QJsonObject json;
    json["model"] = modelInfo.modelId;
    json["messages"] = QJsonArray{
        QJsonObject{{"role", "system"}, {"content", data.manifest}},
        QJsonObject{{"role", "user"}, {"content", userJson}},
    };

    return QJsonDocument(json).toJson();
}

AiResponse OpenAiCompletionsApi::interpretResponse(const QByteArray &response)
{
    using Error = AiResponse::Error;

    if (response.isEmpty())
        return Error::EmptyResponse;

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(response, &parseError);
    if (parseError.error != QJsonParseError::NoError)
        return Error::JsonParseError;

    QJsonObject rootObject = doc.object();

    if (rootObject.isEmpty())
        return Error::EmptyResponse;

    if (!rootObject.contains("choices") || !rootObject["choices"].isArray())
        return AiResponse::structureError("Missing or invalid `choices` array");

    QJsonArray choicesArray = rootObject["choices"].toArray();
    if (choicesArray.isEmpty())
        return AiResponse::structureError("`choices` array is empty");

    QJsonObject firstChoice = choicesArray.first().toObject();
    if (!firstChoice.contains("message") || !firstChoice["message"].isObject())
        return AiResponse::structureError("Missing or invalid `message` object in first `choice`");

    QJsonObject messageObject = firstChoice["message"].toObject();
    QString content = messageObject.value("content").toString();
    if (content.isEmpty())
        return Error::EmptyMessage;

    return AiApiUtils::aiResponseFromContent(content);
}

bool OpenAiCompletionsApi::accepts(const AiModelInfo &info)
{
    return info.isValid() && info.url.toString().contains("/chat/completions", Qt::CaseInsensitive);
}

} // namespace QmlDesigner
