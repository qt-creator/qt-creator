// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "searchresulthighlighter.h"

#include <utils/theme/theme.h>

namespace Core {

SearchResultHighlighter::SearchResultHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{

}

void SearchResultHighlighter::setSearchExpression(const QRegularExpression &expr)
{
    m_searchExpr = expr;
    rehighlight();
}

void SearchResultHighlighter::highlightBlock(const QString &text)
{
    if (m_searchExpr.pattern().isEmpty())
        return;
    QTextCharFormat format;
    format.setBackground(creatorColor(Utils::Theme::OutputPanes_SearchResultBackgroundColor));

    int idx = 0;
    int l = 0;
    while (idx < text.size()) {
        const QRegularExpressionMatch match = m_searchExpr.match(text, idx + l);
        if (!match.hasMatch())
            break;
        idx = match.capturedStart();
        l = match.capturedEnd() - idx;
        if (l == 0)
            ++l; // avoid infinite loop on empty matches
        else
            setFormat(idx, match.capturedLength(), format);
    }
}
} // namespace Core
