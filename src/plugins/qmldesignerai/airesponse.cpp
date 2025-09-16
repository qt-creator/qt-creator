// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "airesponse.h"

#include <qmldesignertr.h>

#include <QJsonArray>
#include <QJsonDocument>

namespace QmlDesigner {

QString AiResponseFile::filePath() const
{
    return m_content.value("filePath").toString();
}

QString AiResponseFile::content() const
{
    return m_content.value("content").toString();
}

bool AiResponseFile::isValid() const
{
    return !m_content.isEmpty();
}

AiResponseFile::AiResponseFile(const QJsonObject &json)
    : m_content(json)
{}

AiResponse::AiResponse(const QByteArray &response)
{
    if (response.isEmpty()) {
        setError(Error::EmptyResponse);
        return;
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(response, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        setError(Error::JsonParseError);
        return;
    }

    m_rootObject = doc.object();

    const QString contentStr = getContent();
    if (contentStr.isEmpty())
        return;

    parseContent(contentStr);
}

QString AiResponse::errorString() const
{
    switch (error()) {
    case Error::NoError:
        return {};
    case Error::JsonParseError:
        return Tr::tr("JSON parse error");
    case Error::EmptyResponse:
        return Tr::tr("Empty JSON response");
    case Error::InvalidChoices:
        return Tr::tr("Missing or invalid `choices` array");
    case Error::EmptyChoices:
        return Tr::tr("`choices` array is empty");
    case Error::InvalidMessage:
        return Tr::tr("Missing or invalid `message` object in first `choice`");
    case Error::EmptyMessage:
        return Tr::tr("Missing or invalid `content` string in `message`");
    case Error::InvalidContentStructure:
        return Tr::tr("Invalid `content` structure received by AI");
    }

    const int errorNo = static_cast<int>(error());
    qWarning() << __FUNCTION__ << "Error has no string " << errorNo;
    return Tr::tr("Error number %1").arg(errorNo);
}

AiResponseFile AiResponse::file() const
{
    if (error() != Error::NoError)
        return AiResponseFile{};

    return AiResponseFile(m_content.value("file").toObject());
}

QStringList AiResponse::selectedIds() const
{
    if (error() != Error::NoError)
        return {};

    QStringList result;
    const QJsonArray idArray = m_content.value("select").toArray();
    result.reserve(idArray.size());
    for (const QJsonValue &value : idArray)
        result.append(value.toString());

    return result;
}

AiResponse::Error AiResponse::error() const
{
    return m_error;
}

void AiResponse::setError(Error error)
{
    m_error = error;
}

QString AiResponse::getContent()
{
    auto setErrorAndReturn = [this](Error error) -> QString {
        setError(error);
        return {};
    };

    if (m_rootObject.isEmpty())
        return setErrorAndReturn(Error::EmptyResponse);

    if (!m_rootObject.contains("choices") || !m_rootObject["choices"].isArray())
        return setErrorAndReturn(Error::InvalidChoices);

    QJsonArray choicesArray = m_rootObject["choices"].toArray();
    if (choicesArray.isEmpty())
        return setErrorAndReturn(Error::EmptyChoices);

    QJsonObject firstChoice = choicesArray.first().toObject();
    if (!firstChoice.contains("message") || !firstChoice["message"].isObject())
        return setErrorAndReturn(Error::InvalidMessage);

    QJsonObject messageObject = firstChoice["message"].toObject();
    QString contentString = messageObject.value("content").toString();
    if (contentString.isEmpty())
        return setErrorAndReturn(Error::EmptyMessage);

    return contentString;
}

void AiResponse::parseContent(const QString &content)
{
    if (error() != Error::NoError)
        return;

    QString contentString = content;

    // remove <think> block if exists
    if (contentString.startsWith("<think>")) {
        int endPos = contentString.indexOf("</think>");
        if (endPos != -1)
            contentString.remove(0, endPos + 8);
        else                            // If no closing tag, remove the opening tag only
            contentString.remove(0, 7); // 7 is length of "<think>"
    }

    contentString = contentString.trimmed();

    // remove the start/end sentence and ``` if exists
    if (contentString.startsWith("```json", Qt::CaseInsensitive) && contentString.endsWith("```")) {
        contentString.remove(0, 7);
        contentString.chop(3);

        QJsonDocument doc = QJsonDocument::fromJson(contentString.toUtf8());
        if (!doc.isObject())
            setError(Error::InvalidContentStructure);

        contentFromObject(doc.object());
    } else if (contentString.startsWith("```qml", Qt::CaseInsensitive) && contentString.endsWith("```")) {
        contentString.remove(0, 3);
        contentString.chop(3);

        QJsonObject virtualContent {
            {"content", contentString},
        };

        contentFromObject(virtualContent);
    } else {
        setError(Error::InvalidContentStructure);
    }
}

void AiResponse::contentFromObject(const QJsonObject &jsonObject)
{
    if (error() != Error::NoError)
        return;

    if (jsonObject.isEmpty())
        return setError(Error::InvalidContentStructure);

    m_content = jsonObject;
}

} // namespace QmlDesigner
