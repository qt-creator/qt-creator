// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QMetaType>
#include <QStringList>

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

    AiResponse();
    AiResponse(Error error);

    QString qml() const;
    Error error() const;
    QString errorString() const;
    QStringList selectedIds() const;

    void setError(Error error);
    void setQml(const QString &qml);
    void setSelectedIds(const QStringList &ids);

private:
    Error m_error = Error::NoError;
    QString m_qml;
    QStringList m_selectedIds;
};

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::AiResponse);
