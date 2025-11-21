// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "openairesponsesapi.h"

#include "aiapiutils.h"
#include "airesponse.h"

#include <stringutils.h>
#include <utils/filepath.h>

#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QRegularExpression>

namespace QmlDesigner {

namespace {

QJsonObject toJsonImage(const Utils::FilePath &imagePath)
{
    using namespace Qt::StringLiterals;
    QImage image(imagePath.toFSPathString());
    const QByteArray imageFormat = imagePath.suffix().toLatin1();
    return {
        {"type", "input_image"},
        {"image_url",
         "data:image/%1;base64,%2"_L1
             .arg(imageFormat, AiApiUtils::toBase64Image(image, imageFormat))},
    };
}

QJsonArray getUserContentArray(const QUrl &imageUrl, const QString &currentQml, const QString &prompt)
{
    using namespace Qt::StringLiterals;
    using Utils::FilePath;

    QJsonArray contentArray;
    contentArray << QJsonObject{
        {"type", "input_text"},
        {"text",
         "Current Qml: ```qml\n%1\n```\n"
         "Request: \"%2\""_L1.arg(currentQml, prompt)},
    };

    if (!imageUrl.isEmpty()) {
        FilePath imagePath = FilePath::fromUrl(imageUrl);
        if (imagePath.exists())
            contentArray << toJsonImage(imagePath);
    }

    return contentArray;
}

} // namespace

/*!
 * \class OpenAiResponsesApi
 * \brief Reimplements AbstractAiApi for interacting with the OpenAI Responses API.
 * \see https://platform.openai.com/docs/api-reference/responses
 */

void OpenAiResponsesApi::setRequestHeader(
    QNetworkRequest *networkRequest, const AiModelInfo &modelInfo)
{
    networkRequest->setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    networkRequest
        ->setRawHeader("Authorization", QByteArray("Bearer ").append(modelInfo.apiKey.toUtf8()));
}

QByteArray OpenAiResponsesApi::createContent(const Data &data, const AiModelInfo &modelInfo)
{
    QJsonArray userContentArray = getUserContentArray(data.attachedImage, data.qml, data.userPrompt);

    QJsonObject json = {
        {"model", modelInfo.modelId},
        {"instructions", data.manifest},
        {
            "input",
            QJsonArray{
                QJsonObject{
                    {"role", "user"},
                    {"content", userContentArray},
                },
            },
        },
    };

    return QJsonDocument(json).toJson();
}

AiResponse OpenAiResponsesApi::interpretResponse(const QByteArray &response)
{
    using Error = AiResponse::Error;

    if (response.isEmpty())
        return Error::EmptyResponse;

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(response, &parseError);
    if (parseError.error != QJsonParseError::NoError)
        return Error::JsonParseError;

    const QJsonObject rootObject = doc.object();

    if (rootObject.isEmpty())
        return Error::EmptyResponse;

    const QJsonValue outputValue = rootObject.value("output");
    if (!outputValue.isArray())
        return AiResponse::structureError("`output` object not found.");
    const QJsonArray outputArray = outputValue.toArray();

    QJsonObject assistantReply;
    for (const QJsonValue &outputItem : outputArray) {
        const QJsonObject outputObject = outputItem.toObject();
        if (outputObject.isEmpty() || !outputObject.contains("role"))
            continue;

        const QString roleValue = outputObject.value("role").toString();
        if (roleValue == "assistant") {
            assistantReply = outputObject;
            break;
        }
    }
    if (assistantReply.isEmpty())
        return AiResponse::structureError("`output` doesn't contain assistant data.");

    const QJsonValue contentValue = assistantReply.value("content");
    if (!contentValue.isArray())
        return AiResponse::structureError("`content` array not found.");

    const QJsonArray contentArray = contentValue.toArray();
    if (contentArray.isEmpty())
        return AiResponse::structureError("`content` array is empty.");

    QString firstTextContent;
    for (const QJsonValue &contentItem : contentArray) {
        const QJsonObject contentObject = contentItem.toObject();
        const QString text = contentObject.value("text").toString();
        if (!contentObject.isEmpty() && contentObject.value("type").toString() == "output_text"
            && !text.isEmpty()) {
            firstTextContent = text;
            break;
        }
    }

    if (firstTextContent.isEmpty())
        return Error::EmptyMessage;

    return AiApiUtils::aiResponseFromContent(firstTextContent);
}

bool OpenAiResponsesApi::accepts(const AiModelInfo &info)
{
    if (!info.isValid())
        return false;

    static const QRegularExpression regex{R"(\/responses\/?$)"};
    return info.url.toString().contains(regex);
}

} // namespace QmlDesigner
