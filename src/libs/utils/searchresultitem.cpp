// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "searchresultitem.h"

namespace Utils {

int Search::TextRange::length(const QString &text) const
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

SearchResultColor::SearchResultColor(const QColor &textBg, const QColor &textFg,
                                     const QColor &highlightBg, const QColor &highlightFg,
                                     const QColor &functionBg, const QColor &functionFg)
    : textBackground(textBg)
    , textForeground(textFg)
    , highlightBackground(highlightBg)
    , highlightForeground(highlightFg)
    , containingFunctionBackground(functionBg)
    , containingFunctionForeground(functionFg)
{
    if (!highlightBackground.isValid())
        highlightBackground = textBackground;
    if (!highlightForeground.isValid())
        highlightForeground = textForeground;
    if (!containingFunctionBackground.isValid())
        containingFunctionBackground = textBackground;
    if (!containingFunctionForeground.isValid())
        containingFunctionForeground = textForeground;
}

static QString displayText(const QString &line)
{
    QString result = line;
    auto end = result.end();
    for (auto it = result.begin(); it != end; ++it) {
        if (!it->isSpace() && !it->isPrint())
            *it = QChar('?');
    }
    return result;
}

void SearchResultItem::setDisplayText(const QString &text)
{
    setLineText(displayText(text));
}

void SearchResultItem::setMainRange(int line, int column, int length)
{
    m_mainRange = {{line, column}, {line, column + length}};
}

QTCREATOR_UTILS_EXPORT size_t qHash(SearchResultColor::Style style, uint seed)
{
    int a = int(style);
    return ::qHash(a, seed);
}

bool Search::TextPosition::operator==(const Search::TextPosition &other) const
{
    return line == other.line && column == other.column;
}

bool Search::TextRange::operator==(const Search::TextRange &other) const
{
    return begin == other.begin && end == other.end;
}

bool SearchResultItem::operator==(const SearchResultItem &other) const
{
    return m_path == other.m_path && m_lineText == other.m_lineText
        && m_userData == other.m_userData && m_mainRange == other.m_mainRange
        && m_useTextEditorFont == other.m_useTextEditorFont
        && m_selectForReplacement == other.m_selectForReplacement && m_style == other.m_style
        && m_containingFunctionName == other.m_containingFunctionName;
}

} // namespace Utils
