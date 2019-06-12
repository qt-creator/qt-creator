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

QTextLayout::FormatRange rangeForResult(const HighlightingResult &result,
                                        const QHash<int, QTextCharFormat> &kindToFormat)
{
    QTextLayout::FormatRange formatRange;

    formatRange.start = int(result.column) - 1;
    formatRange.length = int(result.length);
    formatRange.format = result.useTextSyles
        ? TextEditorSettings::fontSettings().toTextCharFormat(result.textStyles)
        : kindToFormat.value(result.kind);
    return formatRange;
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

    const int firstResultBlockNumber = int(future.resultAt(from).line) - 1;

    // blocks between currentBlockNumber and the last block with results will
    // be cleaned of additional extra formats if they have no results
    int currentBlockNumber = 0;
    for (int i = from - 1; i >= 0; --i) {
        const HighlightingResult &result = future.resultAt(i);
        const int blockNumber = int(result.line) - 1;
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
        const int blockNumber = int(result.line) - 1;
        QTC_ASSERT(blockNumber < doc->blockCount(), return);

        // clear formats of blocks until blockNumber
        while (currentBlockNumber < blockNumber) {
            highlighter->clearExtraFormats(b);
            b = b.next();
            ++currentBlockNumber;
        }

        // collect all the formats for the current line
        QVector<QTextLayout::FormatRange> formats;
        formats.reserve(to - from);
        forever {
            const QTextLayout::FormatRange formatRange = rangeForResult(result, kindToFormat);
            if (formatRange.format.isValid())
                formats.append(formatRange);

            ++i;
            if (i >= to)
                break;
            result = future.resultAt(i);
            const int nextBlockNumber = int(result.line) - 1;
            if (nextBlockNumber != blockNumber)
                break;
        }
        highlighter->setExtraFormats(b, std::move(formats));
        b = b.next();
        ++currentBlockNumber;
    }
}

void SemanticHighlighter::setExtraAdditionalFormats(SyntaxHighlighter *highlighter,
                                                    const QList<HighlightingResult> &results,
                                                    const QHash<int, QTextCharFormat> &kindToFormat)
{
    if (!highlighter)
        return;
    highlighter->clearAllExtraFormats();

    QTextDocument *doc = highlighter->document();
    QTC_ASSERT(doc, return );

    QVector<QVector<QTextLayout::FormatRange>> ranges(doc->blockCount());

    for (auto result : results) {
        const QTextLayout::FormatRange formatRange = rangeForResult(result, kindToFormat);
        if (formatRange.format.isValid())
            ranges[int(result.line) - 1].append(formatRange);
    }

    for (int blockNumber = 0; blockNumber < ranges.count(); ++blockNumber) {
        if (!ranges[blockNumber].isEmpty()) {
            QTextBlock b = doc->findBlockByNumber(blockNumber);
            QTC_ASSERT(b.isValid(), return );
            highlighter->setExtraFormats(b, std::move(ranges[blockNumber]));
        }
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
            lastBlockNumber = int(result.line) - 1;
            break;
        }
    }

    QTextDocument *doc = highlighter->document();

    const int firstBlockToClear = lastBlockNumber + 1;
    if (firstBlockToClear >= doc->blockCount())
        return;

    QTextBlock b = doc->findBlockByNumber(firstBlockToClear);

    while (b.isValid()) {
        highlighter->clearExtraFormats(b);
        b = b.next();
    }
}
