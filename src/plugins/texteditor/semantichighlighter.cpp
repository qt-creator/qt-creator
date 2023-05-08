// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "semantichighlighter.h"

#include "syntaxhighlighter.h"
#include "texteditorsettings.h"
#include "syntaxhighlighterrunner.h"

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

void SemanticHighlighter::incrementalApplyExtraAdditionalFormats(
    BaseSyntaxHighlighterRunner *highlighter,
    const QFuture<HighlightingResult> &future,
    int from,
    int to,
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

    QList<int> clearBlockNumberVector;
    QMap<int, QList<QTextLayout::FormatRange>> blockNumberMap;
    for (auto &[block, ranges] : formatRanges) {
        while (currentBlock < block) {
            clearBlockNumberVector.append(currentBlock.blockNumber());
            currentBlock = currentBlock.next();
        }
        blockNumberMap[block.blockNumber()] = ranges;
        currentBlock = block.next();
    }
    highlighter->clearExtraFormats(clearBlockNumberVector);
    highlighter->setExtraFormats(blockNumberMap);
}

void SemanticHighlighter::setExtraAdditionalFormats(BaseSyntaxHighlighterRunner *highlighter,
                                                    const QList<HighlightingResult> &results,
                                                    const QHash<int, QTextCharFormat> &kindToFormat)
{
    if (!highlighter)
        return;
    highlighter->clearAllExtraFormats();

    QTextDocument *doc = highlighter->document();
    QTC_ASSERT(doc, return );

    QMap<int, QList<QTextLayout::FormatRange>> blockNumberMap;

    for (const HighlightingResult &result : results) {
        const Ranges ranges = rangesForResult(result, doc, kindToFormat);
        for (const Range &range : ranges)
            blockNumberMap[range.block.blockNumber()].append(range.formatRange);
    }

    highlighter->setExtraFormats(blockNumberMap);
}

void SemanticHighlighter::clearExtraAdditionalFormatsUntilEnd(
    BaseSyntaxHighlighterRunner *highlighter, const QFuture<HighlightingResult> &future)
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

    QList<int> clearBlockNumberVector;
    for (QTextBlock b = firstBlockToClear; b.isValid(); b = b.next())
        clearBlockNumberVector.append(b.blockNumber());

    highlighter->clearExtraFormats(clearBlockNumberVector);
}
