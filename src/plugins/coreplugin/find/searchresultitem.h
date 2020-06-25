/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <utils/hostosinfo.h>

#include <QIcon>
#include <QStringList>
#include <QVariant>

namespace Core {

namespace Search {

class TextPosition
{
public:
    TextPosition() = default;
    TextPosition(int line, int column) : line(line), column(column) {}

    int line = -1; // (0 or -1 for no line number)
    int column = -1; // 0-based starting position for a mark (-1 for no mark)

    bool operator<(const TextPosition &other)
    { return line < other.line || (line == other.line && column < other.column); }
};

class TextRange
{
public:
    TextRange() = default;
    TextRange(TextPosition begin, TextPosition end) : begin(begin), end(end) {}

    QString mid(const QString &text) const { return text.mid(begin.column, length(text)); }

    int length(const QString &text) const
    {
        if (begin.line == end.line)
            return end.column - begin.column;

        const int lineCount = end.line - begin.line;
        int index = text.indexOf(QChar::LineFeed);
        int currentLine = 1;
        while (index > 0 && currentLine < lineCount) {
            ++index;
            index = text.indexOf(QChar::LineFeed, index);
            ++currentLine;
        }

        if (index < 0)
            return 0;

        return index - begin.column + end.column;
    }

    TextPosition begin;
    TextPosition end;

    bool operator<(const TextRange &other)
    { return begin < other.begin; }
};

} // namespace Search

class SearchResultItem
{
public:
    QStringList path; // hierarchy to the parent item of this item
    QString text; // text to show for the item itself
    QIcon icon; // icon to show in front of the item (by be null icon to hide)
    QVariant userData; // user data for identification of the item
    Search::TextRange mainRange;
    bool useTextEditorFont = false;
};

} // namespace Core

Q_DECLARE_METATYPE(Core::SearchResultItem)
Q_DECLARE_METATYPE(Core::Search::TextPosition)
