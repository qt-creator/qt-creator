// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

const Ranges rangesForResult(const HighlightingResult &result, const QTextBlock &startBlock,
                             const QHash<int, QTextCharFormat> &kindToFormat)
{
    const QTextCharFormat format = result.useTextSyles
        ? TextEditorSettings::fontSettings().toTextCharFormat(result.textStyles)
        : kindToFormat.value(result.kind);
    if (!format.isValid())
        return {};

    HighlightingResult curResult = result;
    QTextBlock curBlock = startBlock;
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

const Ranges rangesForResult(
        const HighlightingResult &result,
        QTextDocument *doc,
        const QHash<int, QTextCharFormat> &kindToFormat,
        const Splitter &splitter = {})
{
    const QTextBlock startBlock = doc->findBlockByNumber(result.line - 1);
    if (splitter) {
        Ranges ranges;
        for (const auto &[newResult, newBlock] : splitter(result, startBlock))
            ranges << rangesForResult(newResult, newBlock, kindToFormat);
        return ranges;
    }
    return rangesForResult(result, startBlock, kindToFormat);
}

}

void SemanticHighlighter::incrementalApplyExtraAdditionalFormats(SyntaxHighlighter *highlighter,
        const QFuture<HighlightingResult> &future,
        int from, int to,
        const QHash<int, QTextCharFormat> &kindToFormat,
        const Splitter &splitter)
{
    if (to <= from)
        return;

    const int resultStartLine = future.resultAt(from).line;
    int formattingStartLine = 1;

    // Find the line on which to start formatting, where "formatting" means to either
    // clear out formats from outdated document versions (if there is no current result
    // on that line), or apply the format corresponding to the respective result.
    // Note that if there are earlier results on the same line, we have to make sure they
    // get re-applied by adapting the from variable accordingly.
    for (int i = from - 1; i >= 0; --i) {
        const HighlightingResult &result = future.resultAt(i);
        if (result.line == resultStartLine) {
            from = i;
        } else if (result.line < resultStartLine) {
            formattingStartLine = result.line + 1;
            from = i + 1;
            break;
        }
    }

    QTextDocument *doc = highlighter->document();
    QTC_ASSERT(formattingStartLine <= doc->blockCount(), return);
    QTextBlock currentBlock = doc->findBlockByNumber(formattingStartLine - 1);

    std::map<QTextBlock, QVector<QTextLayout::FormatRange>> formatRanges;
    for (int i = from; i < to; ++i) {
        for (const Range &range : rangesForResult(future.resultAt(i), doc, kindToFormat, splitter))
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
        for (const Range &range : rangesForResult(result, doc, kindToFormat))
            formatRanges[range.block].append(range.formatRange);
    }

    for (auto &[block, ranges] : formatRanges)
        highlighter->setExtraFormats(block, std::move(ranges));
}

void SemanticHighlighter::clearExtraAdditionalFormatsUntilEnd(
        SyntaxHighlighter *highlighter,
        const QFuture<HighlightingResult> &future)
{
    const QTextDocument * const doc = highlighter->document();
    QTextBlock firstBlockToClear = doc->begin();
    for (int i = future.resultCount() - 1; i >= 0; --i) {
        const HighlightingResult &result = future.resultAt(i);
        if (result.line) {
            const QTextBlock blockForLine = doc->findBlockByNumber(result.line - 1);
            const QTextBlock lastBlockWithResults = doc->findBlock(
                        blockForLine.position() + result.column - 1 + result.length);
            firstBlockToClear = lastBlockWithResults.next();
            break;
        }
    }

    for (QTextBlock b = firstBlockToClear; b.isValid(); b = b.next())
        highlighter->clearExtraFormats(b);
}
