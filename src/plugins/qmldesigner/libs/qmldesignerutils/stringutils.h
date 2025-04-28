// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QString>
#include <QStringView>

#include <algorithm>
#include <ranges>

namespace QmlDesigner::StringUtils {

inline QString escape(const QString &value)
{
    using namespace Qt::StringLiterals;

    if (value.length() == 6 && value.startsWith("\\u")) //Do not double escape unicode chars
        return value;

    QString result = value;

    result.replace("\\"_L1, "\\\\"_L1);
    result.replace("\""_L1, "\\\""_L1);
    result.replace("\t"_L1, "\\t"_L1);
    result.replace("\r"_L1, "\\r"_L1);
    result.replace("\n"_L1, "\\n"_L1);

    return result;
}

inline QString deescape(const QString &value)
{
    using namespace Qt::StringLiterals;

    if (value.length() == 6 && value.startsWith("\\u")) //Ignore unicode chars
        return value;

    QString result = value;

    result.replace("\\\\"_L1, "\\"_L1);
    result.replace("\\\""_L1, "\""_L1);
    result.replace("\\t"_L1, "\t"_L1);
    result.replace("\\r"_L1, "\r"_L1);
    result.replace("\\n"_L1, "\n"_L1);

    return result;
}

template<typename T>
concept is_object = std::is_object_v<T>;

std::pair<QStringView, QStringView> split_last(is_object auto &&, QChar c) = delete; // remove rvalue overload

inline std::pair<QStringView, QStringView> split_last(QStringView text, QChar c)
{
    auto splitPoint = std::ranges::find(text | std::views::reverse, c).base();

    if (splitPoint == text.begin())
        return {{}, text};

    return {{text.begin(), std::prev(splitPoint)}, {splitPoint, text.end()}};
}

} // namespace QmlDesigner::StringUtils
