// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QString>

namespace QmlDesigner {

class AiResponse
{
public:
    enum class Error {
        NoError,
        JsonParseError,
        EmptyResponse,
        EmptyMessage,
        RequestError,
        StructureError,
    };

    AiResponse(const QString &text);
    AiResponse(Error error, const QString &errorMessage = {});

    QString text() const;
    Error error() const;
    QString errorMessage() const;

    [[nodiscard]] static AiResponse requestError(const QString &erroMsg);
    [[nodiscard]] static AiResponse structureError(const QString &erroMsg);

private:
    Error m_error = Error::NoError;
    QString m_text;
    QString m_errorMessage;
};

} // namespace QmlDesigner
