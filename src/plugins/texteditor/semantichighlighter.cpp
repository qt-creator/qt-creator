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

#include <QTextBlock>
#include <QTextDocument>

#include <algorithm>

using namespace TextEditor;
using namespace TextEditor::SemanticHighlighter;

namespace {

class Range {
public:
    QTextLayout::FormatRange formatRange;
    QTextBlock block;
};
using Ranges = QVector<Range>;

Ranges rangesForResult(const HighlightingResult &result,
                       QTextDocument *doc,
                       const QHash<int, QTextCharFormat> &kindToFormat)
{
    const QTextCharFormat format = result.useTextSyles
        ? TextEditorSettings::fontSettings().toTextCharFormat(result.textStyles)
        : kindToFormat.value(result.kind);
    if (!format.isValid())
        return {};

    HighlightingResult curResult = result;
    QTextBlock curBlock = doc->findBlockByNumber(curResult.line - 1);
    Ranges ranges;
    while (curBlock.isValid()) {
        Range range;
        range.block = curBlock;
        range.formatRange.format = format;
        range.formatRange.start = curResult.column - 1;
        range.formatRange.length = std::min(curResult.length,
                                            curBlock.length() - range.formatRange.start);
        ranges << range;
        if (range.formatRange.length == curResult.length)
            break;
        curBlock = curBlock.next();
        curResult.column = 1;
        curResult.length -= range.formatRange.length;
    }

    return ranges;
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
    QTextBlock currentBlock = doc->findBlockByNumber(currentBlockNumber);

    std::map<QTextBlock, QVector<QTextLayout::FormatRange>> formatRanges;
    for (int i = from; i < to; ++i) {
        const Ranges ranges = rangesForResult(future.resultAt(i), doc, kindToFormat);
        for (const Range &range : ranges)
            formatRanges[range.block].append(range.formatRange);
    }

    for (auto &[block, ranges] : formatRanges) {
        while (currentBlock < block) {
            highlighter->clearExtraFormats(currentBlock);
            currentBlock = currentBlock.next();
        }
        highlighter->setExtraFormats(block, std::move(ranges));
        currentBlock = block.next();
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

    std::map<QTextBlock, QVector<QTextLayout::FormatRange>> formatRanges;

    for (auto result : results) {
        const Ranges ranges = rangesForResult(result, doc, kindToFormat);
        for (const Range &range : ranges)
            formatRanges[range.block].append(range.formatRange);
    }

    for (auto &[block, ranges] : formatRanges)
        highlighter->setExtraFormats(block, std::move(ranges));
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
