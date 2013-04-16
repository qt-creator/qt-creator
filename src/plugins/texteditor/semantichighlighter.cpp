/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "semantichighlighter.h"

#include "syntaxhighlighter.h"

#include <utils/qtcassert.h>

#include <QTextDocument>
#include <QTextBlock>

using namespace TextEditor;
using namespace TextEditor::SemanticHighlighter;

void TextEditor::SemanticHighlighter::incrementalApplyExtraAdditionalFormats(
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
            QList<QTextLayout::FormatRange> noFormats;
            highlighter->setExtraAdditionalFormats(b, noFormats);
            b = b.next();
            ++currentBlockNumber;
        }

        // collect all the formats for the current line
        QList<QTextLayout::FormatRange> formats;
        formats.reserve(to - from);
        forever {
            QTextLayout::FormatRange formatRange;

            formatRange.format = kindToFormat.value(result.kind);
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
        highlighter->setExtraAdditionalFormats(b, formats);
        b = b.next();
        ++currentBlockNumber;
    }
}

void TextEditor::SemanticHighlighter::clearExtraAdditionalFormatsUntilEnd(
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
    if (firstBlockToClear == doc->blockCount())
        return;
    QTC_ASSERT(firstBlockToClear < doc->blockCount(), return);

    QTextBlock b = doc->findBlockByNumber(firstBlockToClear);

    while (b.isValid()) {
        QList<QTextLayout::FormatRange> noFormats;
        highlighter->setExtraAdditionalFormats(b, noFormats);
        b = b.next();
    }
}
