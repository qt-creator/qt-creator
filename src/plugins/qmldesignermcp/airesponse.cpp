// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "airesponse.h"

#include <qmldesignertr.h>

#include <QDebug>
#include <QRegularExpression>

namespace QmlDesigner {

AiResponse::AiResponse(const QString &text)
    : m_text(text) {};

AiResponse::AiResponse(Error error, const QString &erroMsg)
    : m_error(error)
    , m_errorMessage(erroMsg) {};

QString AiResponse::errorMessage() const
{
    switch (error()) {
    case Error::NoError:
        return {};
    case Error::JsonParseError:
        return Tr::tr("JSON parse error");
    case Error::EmptyResponse:
        return Tr::tr("Empty JSON response");
    case Error::EmptyMessage:
        return Tr::tr("Missing or invalid `content` string in `message`");
    case Error::RequestError:
        return Tr::tr("Request error: %1").arg(m_errorMessage);
    case Error::StructureError:
        return Tr::tr("Structure error: %1").arg(m_errorMessage);
    }

    const int errorNo = static_cast<int>(error());
    qWarning() << __FUNCTION__ << "Error has no string " << errorNo;
    return Tr::tr("Error number %1").arg(errorNo);
}

AiResponse::Error AiResponse::error() const
{
    return m_error;
}

AiResponse AiResponse::requestError(const QString &erroMsg)
{
    return AiResponse(Error::RequestError, erroMsg);
}

AiResponse AiResponse::structureError(const QString &erroMsg)
{
    return AiResponse(Error::StructureError, erroMsg);
}

QString AiResponse::text() const
{
    return m_text;
}

} // namespace QmlDesigner
