// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "stringutils.h"

namespace QmlDesigner::StringUtils {

QString escape(QStringView text)
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

QString deescape(QStringView text)
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

namespace {
enum class Comment { No, Line, Block };

Comment commentKind(QStringView text)
{
    if (text.size() < 2)
        return Comment::No;

    if (text.startsWith(u"//"))
        return Comment::Line;

    if (text.startsWith(u"/*"))
        return Comment::Block;

    return Comment::No;
}
} // namespace

QStringView::iterator find_comment_end(QStringView text)
{
    auto current = text.begin();
    const auto end = text.end();

    while (current != end) {
        text = {current, text.end()};
        switch (commentKind(text)) {
        case Comment::No:
            return current;
        case Comment::Line:
            current = std::ranges::find(text, u'\n');
            break;
        case Comment::Block:
            current = std::ranges::search(text, QStringView{u"*/"}).end();
            current = std::ranges::find(current, end, u'\n');
            break;
        }

        if (current != end)
            current = std::next(current);
    }

    return current;
}

} // namespace QmlDesigner::StringUtils
