// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "openaicompletionsapi.h"

#include "airesponse.h"

#include <utils/filepath.h>

#include <QBuffer>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>

namespace QmlDesigner {

namespace {

QString toBase64Image(const Utils::FilePath &imagePath)
{
    using namespace Qt::StringLiterals;
    QImage image(imagePath.toFSPathString());
    QByteArray byteArray;
    QBuffer buffer(&byteArray);

    image.save(&buffer, imagePath.suffix().toLatin1());

    return "data:image/%1;base64,%2"_L1.arg(imagePath.suffix(), byteArray.toBase64());
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
        if (imagePath.exists()) {
            jsonContent << QJsonObject{
                {"type", "image_url"},
                {"image_url", QJsonObject{{"url", toBase64Image(imagePath)}}},
            };
        }
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
    return AiResponse(response);
}

bool OpenAiCompletionsApi::accepts(const AiModelInfo &info)
{
    return info.isValid();
}

} // namespace QmlDesigner
