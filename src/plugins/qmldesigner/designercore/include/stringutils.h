// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QString>

namespace QmlDesigner {

inline QString escape(const QString &value)
{
    QString result = value;

    if (value.length() == 6 && value.startsWith("\\u")) //Do not double escape unicode chars
        return value;

    result.replace(QStringLiteral("\\"), QStringLiteral("\\\\"));
    result.replace(QStringLiteral("\""), QStringLiteral("\\\""));
    result.replace(QStringLiteral("\t"), QStringLiteral("\\t"));
    result.replace(QStringLiteral("\r"), QStringLiteral("\\r"));
    result.replace(QStringLiteral("\n"), QStringLiteral("\\n"));

    return result;
}

inline QString deescape(const QString &value)
{
    QString result = value;

    if (value.length() == 6 && value.startsWith("\\u")) //Ignore unicode chars
        return value;

    result.replace(QStringLiteral("\\\\"), QStringLiteral("\\"));
    result.replace(QStringLiteral("\\\""), QStringLiteral("\""));
    result.replace(QStringLiteral("\\t"), QStringLiteral("\t"));
    result.replace(QStringLiteral("\\r"), QStringLiteral("\r"));
    result.replace(QStringLiteral("\\n"), QStringLiteral("\n"));

    return result;
}

} // namespace QmlDesigner
