// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QString>

using namespace Qt::StringLiterals;

namespace QmlDesigner {

inline QString escape(const QString &value)
{
    QString result = value;

    if (value.length() == 6 && value.startsWith("\\u")) //Do not double escape unicode chars
        return value;

    result.replace("\\"_L1, "\\\\"_L1);
    result.replace("\""_L1, "\\\""_L1);
    result.replace("\t"_L1, "\\t"_L1);
    result.replace("\r"_L1, "\\r"_L1);
    result.replace("\n"_L1, "\\n"_L1);

    return result;
}

inline QString deescape(const QString &value)
{
    QString result = value;

    if (value.length() == 6 && value.startsWith("\\u")) //Ignore unicode chars
        return value;

    result.replace("\\\\"_L1, "\\"_L1);
    result.replace("\\\""_L1, "\""_L1);
    result.replace("\\t"_L1, "\t"_L1);
    result.replace("\\r"_L1, "\r"_L1);
    result.replace("\\n"_L1, "\n"_L1);

    return result;
}

} // namespace QmlDesigner
