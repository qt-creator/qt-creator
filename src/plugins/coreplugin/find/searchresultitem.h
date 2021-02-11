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

#include "searchresultcolor.h"

#include <utils/fileutils.h>
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

class CORE_EXPORT SearchResultItem
{
public:
    QStringList path() const { return m_path; }
    void setPath(const QStringList &path) { m_path = path; }
    void setFilePath(const Utils::FilePath &filePath)
    {
        m_path = QStringList{filePath.toUserOutput()};
    }

    QString lineText() const { return m_lineText; }
    void setLineText(const QString &text) { m_lineText = text; }

    QIcon icon() const { return m_icon; }
    void setIcon(const QIcon &icon) { m_icon = icon; }

    QVariant userData() const { return m_userData; }
    void setUserData(const QVariant &userData) { m_userData = userData; }

    Search::TextRange mainRange() const { return m_mainRange; }
    void setMainRange(const Search::TextRange &mainRange) { m_mainRange = mainRange; }
    void setMainRange(int line, int column, int length)
    {
        m_mainRange = {};
        m_mainRange.begin.line = line;
        m_mainRange.begin.column = column;
        m_mainRange.end.line = m_mainRange.begin.line;
        m_mainRange.end.column = m_mainRange.begin.column + length;
    }

    bool useTextEditorFont() const { return m_useTextEditorFont; }
    void setUseTextEditorFont(bool useTextEditorFont) { m_useTextEditorFont = useTextEditorFont; }

    SearchResultColor::Style style() const { return m_style; }
    void setStyle(SearchResultColor::Style style) { m_style = style; }

private:
    QStringList m_path; // hierarchy to the parent item of this item
    QString m_lineText; // text to show for the item itself
    QIcon m_icon; // icon to show in front of the item (by be null icon to hide)
    QVariant m_userData; // user data for identification of the item
    Search::TextRange m_mainRange;
    bool m_useTextEditorFont = false;
    SearchResultColor::Style m_style = SearchResultColor::Style::Default;
};

} // namespace Core

Q_DECLARE_METATYPE(Core::SearchResultItem)
Q_DECLARE_METATYPE(Core::Search::TextPosition)
