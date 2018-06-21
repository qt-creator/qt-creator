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

#include "texteditor_global.h"

#include "fontsettings.h"

#include <QHash>
#include <QFuture>
#include <QTextCharFormat>

namespace TextEditor {

class SyntaxHighlighter;

class TEXTEDITOR_EXPORT HighlightingResult {
public:
    unsigned line = 0; // 1-based
    unsigned column = 0; // 1-based
    unsigned length = 0;
    TextStyles textStyles;
    int kind = 0; /// The various highlighters can define their own kind of results.
    bool useTextSyles = false;

    bool isValid() const
    { return line != 0; }

    bool isInvalid() const
    { return line == 0; }

    HighlightingResult() = default;

    HighlightingResult(unsigned line, unsigned column, unsigned length, int kind)
        : line(line), column(column), length(length), kind(kind), useTextSyles(false)
    {}

    HighlightingResult(unsigned line, unsigned column, unsigned length, TextStyles textStyles)
        : line(line), column(column), length(length), textStyles(textStyles), useTextSyles(true)
    {}

    bool operator==(const HighlightingResult& other) const
    {
        return line == other.line
                && column == other.column
                && length == other.length
                && kind == other.kind;
    }
};

namespace SemanticHighlighter {

// Applies the future results [from, to) and applies the extra formats
// indicated by Result::kind and kindToFormat to the correct location using
// SyntaxHighlighter::setExtraAdditionalFormats.
// It is incremental in the sense that it clears the extra additional formats
// from all lines that have no results between the (from-1).line result and
// the (to-1).line result.
// Requires that results of the Future are ordered by line.
void TEXTEDITOR_EXPORT incrementalApplyExtraAdditionalFormats(
        SyntaxHighlighter *highlighter,
        const QFuture<HighlightingResult> &future,
        int from, int to,
        const QHash<int, QTextCharFormat> &kindToFormat);

// Cleans the extra additional formats after the last result of the Future
// until the end of the document.
// Requires that results of the Future are ordered by line.
void TEXTEDITOR_EXPORT clearExtraAdditionalFormatsUntilEnd(
        SyntaxHighlighter *highlighter,
        const QFuture<HighlightingResult> &future);


} // namespace SemanticHighlighter
} // namespace TextEditor
