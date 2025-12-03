// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmldesignerutils_global.h"

#include <utils/smallstringview.h>

#include <QString>
#include <QStringView>

#include <algorithm>
#include <ranges>

namespace QmlDesigner::StringUtils {

QMLDESIGNERUTILS_EXPORT QString escape(QStringView text);

QMLDESIGNERUTILS_EXPORT QString deescape(QStringView text);

template<typename T>
concept is_object = std::is_object_v<T>;

std::pair<QStringView, QStringView> split_last(is_object auto &&, QChar c) = delete; // remove rvalue overload

inline std::pair<QStringView, QStringView> split_last(QStringView text, QChar c = u'.')
{
    auto splitPoint = std::ranges::find(text | std::views::reverse, c).base();

    if (splitPoint == text.begin())
        return {{}, text};

    return {{text.begin(), std::prev(splitPoint)}, {splitPoint, text.end()}};
}

std::pair<QStringView, QStringView> split_last(is_object auto &&, char c) = delete; // remove rvalue overload

inline std::pair<Utils::SmallStringView, Utils::SmallStringView> split_last(Utils::SmallStringView text,
                                                                            char c = '.')
{
    auto splitPoint = std::ranges::find(text.rbegin(), text.rend(), c).base();

    if (splitPoint == text.begin())
        return {{}, text};

    return {{text.begin(), std::prev(splitPoint)}, {splitPoint, text.end()}};
}

QMLDESIGNERUTILS_EXPORT QStringView::iterator find_comment_end(QStringView text);

} // namespace QmlDesigner::StringUtils
