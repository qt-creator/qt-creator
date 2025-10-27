// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QJsonObject>

namespace QmlDesigner {

class AiResponse
{
public:
    enum class Error {
        NoError,
        JsonParseError,
        EmptyResponse,
        InvalidChoices,
        EmptyChoices,
        InvalidMessage,
        EmptyMessage,
    };

    AiResponse(const QByteArray &response);

    QString content() const;
    Error error() const;
    QString errorString() const;
    QStringList selectedIds() const;

private: // functions
    void setError(Error error);
    void parseContent();

private: // variables
    Error m_error = Error::NoError;
    QJsonObject m_rootObject;
    QString m_content;
};

} // namespace QmlDesigner
