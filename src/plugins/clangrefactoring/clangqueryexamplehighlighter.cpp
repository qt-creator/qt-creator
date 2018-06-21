/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "clangqueryexamplehighlighter.h"

#include <sourcerangescontainer.h>

#include <QTextBlock>


namespace ClangRefactoring {

ClangQueryExampleHighlighter::ClangQueryExampleHighlighter()
    : m_marker(*this)
{
    std::array<QTextCharFormat, 5> textFormats;
    textFormats[0].setBackground(QColor("#c9ffc3"));
    textFormats[1].setBackground(QColor("#c3d9ff"));
    textFormats[2].setBackground(QColor("#e5c3ff"));
    textFormats[3].setBackground(QColor("#ffc3cb"));
    textFormats[4].setBackground(QColor("#ffe8c3"));

    m_marker.setTextFormats(std::move(textFormats));

    setNoAutomaticHighlighting(true);
}

void ClangQueryExampleHighlighter::setSourceRanges(ClangBackEnd::SourceRangesContainer &&container)
{
    m_marker.setSourceRanges(container.takeSourceRangeWithTextContainers());
    rehighlight();
}

void ClangQueryExampleHighlighter::highlightBlock(const QString &text)
{
    int currentLineNumber = currentBlock().blockNumber() + 1;
    m_marker.highlightBlock(uint(currentLineNumber), text);
}

} // namespace ClangRefactoring
