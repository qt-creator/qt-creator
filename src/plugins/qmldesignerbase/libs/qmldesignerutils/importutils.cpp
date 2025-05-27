// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "importutils.h"

#include <QRegularExpression>

#include <algorithm>
#include <ranges>

template<>
inline constexpr bool std::ranges::enable_borrowed_range<QStringView> = true;
template<>
inline constexpr bool std::ranges::enable_view<QStringView> = true;

namespace QmlDesigner::ImportUtils {

using namespace Qt::StringLiterals;

std::optional<QStringView::iterator> find_import_location(QStringView text, QStringView directory)
{
    const auto begin = text.begin();
    const auto headerEnd = std::ranges::find(text, u'{');
    QStringView header = {begin, headerEnd};
    QStringView import{u"import"};

    auto importRange = std::ranges::search(header.rbegin(),
                                           header.rend(),
                                           import.rbegin(),
                                           import.rend());

    if (importRange.empty())
        return {begin};

    auto endLine = std::ranges::find(importRange.begin().base(), headerEnd, u'\n');

    if (endLine != text.end())
        endLine = std::ranges::next(endLine);

    QRegularExpression regex{uR"(^import\s+")"_s + directory + uR"("\s+(?!as))"_s,
                             QRegularExpression::MultilineOption};

    if (regex.matchView({begin, endLine}).hasMatch())
        return {};

    return std::make_optional<QStringView::iterator>(endLine);
}

} // namespace QmlDesigner::ImportUtils
