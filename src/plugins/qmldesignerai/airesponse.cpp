// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "airesponse.h"

#include <qmldesignertr.h>

#include <QDebug>

namespace QmlDesigner {

AiResponse::AiResponse() = default;

AiResponse::AiResponse(Error error)
{
    setError(error);
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
    case Error::RequestError:
        return Tr::tr("Request error: %1").arg(m_errorString);
    case Error::StructureError:
        return Tr::tr("Structure error: %1").arg(m_errorString);
    }

    const int errorNo = static_cast<int>(error());
    qWarning() << __FUNCTION__ << "Error has no string " << errorNo;
    return Tr::tr("Error number %1").arg(errorNo);
}

QStringList AiResponse::selectedIds() const
{
    return m_selectedIds;
}

AiResponse::Error AiResponse::error() const
{
    return m_error;
}

void AiResponse::setError(Error error)
{
    m_error = error;
}

void AiResponse::setQml(const QString &qml)
{
    m_qml = qml;
}

void AiResponse::setSelectedIds(const QStringList &ids)
{
    m_selectedIds = ids;
}

AiResponse AiResponse::requestError(const QString &error)
{
    AiResponse response;
    response.setError(Error::RequestError);
    response.m_errorString = error;
    return response;
}

AiResponse AiResponse::structureError(const QString &error)
{
    AiResponse response;
    response.setError(Error::StructureError);
    response.m_errorString = error;
    return response;
}

QString AiResponse::qml() const
{
    return m_qml;
}

} // namespace QmlDesigner
