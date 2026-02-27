// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QStringList>

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

    AiResponse();
    AiResponse(Error error);

    QString qml() const;
    QString text() const;
    Error error() const;
    QString errorString() const;

    void setError(Error error);
    void setQml(const QString &qml);
    void setText(const QString &text);

    [[nodiscard]] static AiResponse requestError(const QString &error);
    [[nodiscard]] static AiResponse structureError(const QString &error);

private:
    Error m_error = Error::NoError;
    QString m_qml;
    QString m_text;
    QString m_errorString;
};

} // namespace QmlDesigner
