// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "airesponse.h"

#include <qmldesignertr.h>

#include <QJsonArray>
#include <QJsonDocument>

namespace QmlDesigner {

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

    parseContent();
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
    case Error::InvalidQmlBlock:
        return Tr::tr("Invalid QML block");
    }

    const int errorNo = static_cast<int>(error());
    qWarning() << __FUNCTION__ << "Error has no string " << errorNo;
    return Tr::tr("Error number %1").arg(errorNo);
}

QStringList AiResponse::selectedIds() const
{
    QStringList result;
    if (m_content.startsWith("$$") && m_content.endsWith("$$")) {
        QString inner = m_content.mid(2, m_content.length() - 4);
        result = inner.split(",", Qt::SkipEmptyParts);
        for (QString &id : result)
            id = id.trimmed();
    }

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

QString AiResponse::content() const
{
    return m_content;
}

void AiResponse::parseContent()
{
    if (m_rootObject.isEmpty()) {
        setError(Error::EmptyResponse);
        return;
    }

    if (!m_rootObject.contains("choices") || !m_rootObject["choices"].isArray()) {
        setError(Error::InvalidChoices);
        return;
    }

    QJsonArray choicesArray = m_rootObject["choices"].toArray();
    if (choicesArray.isEmpty()) {
        setError(Error::EmptyChoices);
        return;
    }

    QJsonObject firstChoice = choicesArray.first().toObject();
    if (!firstChoice.contains("message") || !firstChoice["message"].isObject()) {
        setError(Error::InvalidMessage);
        return;
    }

    QJsonObject messageObject = firstChoice["message"].toObject();
    m_content = messageObject.value("content").toString();
    if (m_content.isEmpty()) {
        setError(Error::EmptyMessage);
        return;
    }

    // remove <think> block if exists
    if (m_content.startsWith("<think>")) {
        int endPos = m_content.indexOf("</think>");
        if (endPos != -1)
            m_content.remove(0, endPos + 8);
        else                            // If no closing tag, remove the opening tag only
            m_content.remove(0, 7); // 7 is length of "<think>"
    }
    m_content = m_content.trimmed();

    // remove the wrapper "```qml" and "```" if exists
    if (m_content.startsWith("```qml", Qt::CaseInsensitive) && m_content.endsWith("```")) {
        m_content.remove(0, 6);
        m_content.chop(3);
    } else if (!m_content.startsWith("$$")) {
        setError(Error::InvalidQmlBlock);
    }
}

} // namespace QmlDesigner
