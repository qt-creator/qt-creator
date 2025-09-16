// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QJsonObject>

namespace QmlDesigner {

struct AiResponseFile
{
    QString filePath() const;
    QString content() const;
    bool isValid() const;

private:
    friend class AiResponse;
    explicit AiResponseFile() = default;
    explicit AiResponseFile(const QJsonObject &json);

    QJsonObject m_content;
};

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
        InvalidContentStructure,
    };

    AiResponse(const QByteArray &response);

    Error error() const;
    QString errorString() const;
    AiResponseFile file() const;
    QStringList selectedIds() const;

private: // functions
    void setError(Error error);
    QString getContent();
    void parseContent(const QString &content);
    void contentFromObject(const QJsonObject &jsonObject);

private: // variables
    Error m_error = Error::NoError;
    QJsonObject m_rootObject;
    QJsonObject m_content;
};

} // namespace QmlDesigner
