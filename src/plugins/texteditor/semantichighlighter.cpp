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

#include "semantichighlighter.h"

#include "syntaxhighlighter.h"
#include "texteditorsettings.h"

#include <utils/qtcassert.h>

#include <QTextDocument>
#include <QTextBlock>

using namespace TextEditor;
using namespace TextEditor::SemanticHighlighter;

namespace {

QTextCharFormat textCharFormatForResult(const HighlightingResult &result,
                                        const QHash<int, QTextCharFormat> &kindToFormat)
{
    if (result.useTextSyles)
        return TextEditorSettings::fontSettings().toTextCharFormat(result.textStyles);
    else
        return kindToFormat.value(result.kind);
}

}

void SemanticHighlighter::incrementalApplyExtraAdditionalFormats(
        SyntaxHighlighter *highlighter,
        const QFuture<HighlightingResult> &future,
        int from, int to,
        const QHash<int, QTextCharFormat> &kindToFormat)
{
    if (to <= from)
        return;

    const int firstResultBlockNumber = future.resultAt(from).line - 1;

    // blocks between currentBlockNumber and the last block with results will
    // be cleaned of additional extra formats if they have no results
    int currentBlockNumber = 0;
    for (int i = from - 1; i >= 0; --i) {
        const HighlightingResult &result = future.resultAt(i);
        const int blockNumber = result.line - 1;
        if (blockNumber < firstResultBlockNumber) {
            // stop! found where last format stopped
            currentBlockNumber = blockNumber + 1;
            // add previous results for the same line to avoid undoing their formats
            from = i + 1;
            break;
        }
    }

    QTextDocument *doc = highlighter->document();
    QTC_ASSERT(currentBlockNumber < doc->blockCount(), return);
    QTextBlock b = doc->findBlockByNumber(currentBlockNumber);

    HighlightingResult result = future.resultAt(from);
    for (int i = from; i < to && b.isValid(); ) {
        const int blockNumber = result.line - 1;
        QTC_ASSERT(blockNumber < doc->blockCount(), return);

        // clear formats of blocks until blockNumber
        while (currentBlockNumber < blockNumber) {
            QVector<QTextLayout::FormatRange> noFormats;
            highlighter->setExtraFormats(b, noFormats);
            b = b.next();
            ++currentBlockNumber;
        }

        // collect all the formats for the current line
        QVector<QTextLayout::FormatRange> formats;
        formats.reserve(to - from);
        forever {
            QTextLayout::FormatRange formatRange;

            formatRange.format = textCharFormatForResult(result, kindToFormat);
            if (formatRange.format.isValid()) {
                formatRange.start = result.column - 1;
                formatRange.length = result.length;
                formats.append(formatRange);
            }

            ++i;
            if (i >= to)
                break;
            result = future.resultAt(i);
            const int nextBlockNumber = result.line - 1;
            if (nextBlockNumber != blockNumber)
                break;
        }
        highlighter->setExtraFormats(b, formats);
        b = b.next();
        ++currentBlockNumber;
    }
}

void SemanticHighlighter::clearExtraAdditionalFormatsUntilEnd(
        SyntaxHighlighter *highlighter,
        const QFuture<HighlightingResult> &future)
{
    // find block number of last result
    int lastBlockNumber = 0;
    for (int i = future.resultCount() - 1; i >= 0; --i) {
        const HighlightingResult &result = future.resultAt(i);
        if (result.line) {
            lastBlockNumber = result.line - 1;
            break;
        }
    }

    QTextDocument *doc = highlighter->document();

    const int firstBlockToClear = lastBlockNumber + 1;
    if (firstBlockToClear >= doc->blockCount())
        return;

    QTextBlock b = doc->findBlockByNumber(firstBlockToClear);

    while (b.isValid()) {
        QVector<QTextLayout::FormatRange> noFormats;
        highlighter->setExtraFormats(b, noFormats);
        b = b.next();
    }
}
