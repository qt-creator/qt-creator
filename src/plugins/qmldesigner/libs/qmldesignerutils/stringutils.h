// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QString>
#include <QStringView>

#include <algorithm>
#include <ranges>

namespace QmlDesigner::StringUtils {

inline QString escape(QStringView text)
{
    using namespace Qt::StringLiterals;

    if (text.size() == 6 && text.startsWith(u"\\u")) //Do not double escape unicode chars
        return text.toString();

    QString escapedText;
    escapedText.reserve(text.size() * 2);

    const auto end = text.end();
    auto current = text.begin();
    QStringView pattern = u"\\\"\t\r\n";
    while (current != end) {
        auto found = std::ranges::find_first_of(current, end, pattern.begin(), pattern.end());
        escapedText.append(QStringView{current, found});

        if (found == end)
            break;

        QChar c = *found;
        switch (c.unicode()) {
        case u'\\':
            escapedText.append(u"\\\\");
            break;
        case u'\"':
            escapedText.append(u"\\\"");
            break;
        case u'\t':
            escapedText.append(u"\\t");
            break;
        case u'\r':
            escapedText.append(u"\\r");
            break;
        case u'\n':
            escapedText.append(u"\\n");
            break;
        }

        current = std::next(found);
    }

    return escapedText;
}

inline QString deescape(QStringView text)
{
    using namespace Qt::StringLiterals;

    if (text.isEmpty() || (text.size() == 6 && text.startsWith(u"\\u"))) //Ignore unicode chars
        return text.toString();

    QString deescapedText;
    deescapedText.reserve(text.size());

    const auto end = text.end();
    auto current = text.begin();
    while (current != end) {
        auto found = std::ranges::find(current, end, u'\\');
        deescapedText.append(QStringView{current, found});

        if (found == end)
            break;

        current = std::next(found);

        if (current == end) {
            deescapedText.append(u'\\');
            break;
        }

        QChar c = *current;
        switch (c.unicode()) {
        case u'\\':
            deescapedText.append(u'\\');
            current = std::next(current);
            break;
        case u'\"':
            deescapedText.append(u'\"');
            current = std::next(current);
            break;
        case u't':
            deescapedText.append(u'\t');
            current = std::next(current);
            break;
        case u'r':
            deescapedText.append(u'\r');
            current = std::next(current);
            break;
        case u'n':
            deescapedText.append(u'\n');
            current = std::next(current);
            break;
        default:
            deescapedText.append(u'\\');
            break;
        }
    }

    return deescapedText;
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
