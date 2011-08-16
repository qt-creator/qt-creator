/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "semantichighlighter.h"

#include "syntaxhighlighter.h"

#include <utils/qtcassert.h>

#include <QtGui/QTextDocument>
#include <QtGui/QTextBlock>

using namespace TextEditor;
using namespace TextEditor::SemanticHighlighter;

void TextEditor::SemanticHighlighter::incrementalApplyExtraAdditionalFormats(
        SyntaxHighlighter *highlighter,
        const QFuture<Result> &future,
        int from, int to,
        const QHash<int, QTextCharFormat> &kindToFormat)
{
    QMap<int, QVector<Result> > blockNumberToResults;
    for (int i = from; i < to; ++i) {
        const Result &result = future.resultAt(i);
        if (!result.line)
            continue;
        blockNumberToResults[result.line - 1].append(result);
    }
    if (blockNumberToResults.isEmpty())
        return;

    const int firstResultBlockNumber = blockNumberToResults.constBegin().key();

    // blocks between currentBlockNumber and the last block with results will
    // be cleaned of additional extra formats if they have no results
    int currentBlockNumber = 0;
    for (int i = from - 1; i >= 0; --i) {
        const Result &result = future.resultAt(i);
        if (!result.line)
            continue;
        const int blockNumber = result.line - 1;
        if (blockNumber < firstResultBlockNumber) {
            // stop! found where last format stopped
            currentBlockNumber = blockNumber + 1;
            break;
        } else {
            // add previous results for the same line to avoid undoing their formats
            blockNumberToResults[blockNumber].append(result);
        }
    }

    QTextDocument *doc = highlighter->document();
    QTC_ASSERT(currentBlockNumber < doc->blockCount(), return);
    QTextBlock b = doc->findBlockByNumber(currentBlockNumber);

    QMapIterator<int, QVector<Result> > it(blockNumberToResults);
    while (b.isValid() && it.hasNext()) {
        it.next();
        const int blockNumber = it.key();
        QTC_ASSERT(blockNumber < doc->blockCount(), return);

        // clear formats of blocks until blockNumber
        while (currentBlockNumber < blockNumber) {
            highlighter->setExtraAdditionalFormats(b, QList<QTextLayout::FormatRange>());
            b = b.next();
            ++currentBlockNumber;
        }

        QList<QTextLayout::FormatRange> formats;
        foreach (const Result &result, it.value()) {
            QTextLayout::FormatRange formatRange;

            formatRange.format = kindToFormat.value(result.kind);
            if (!formatRange.format.isValid())
                continue;

            formatRange.start = result.column - 1;
            formatRange.length = result.length;
            formats.append(formatRange);
        }
        highlighter->setExtraAdditionalFormats(b, formats);
        b = b.next();
        ++currentBlockNumber;
    }
}

void TextEditor::SemanticHighlighter::clearExtraAdditionalFormatsUntilEnd(
        SyntaxHighlighter *highlighter,
        const QFuture<Result> &future)
{
    // find block number of last result
    int lastBlockNumber = 0;
    for (int i = future.resultCount() - 1; i >= 0; --i) {
        const Result &result = future.resultAt(i);
        if (result.line) {
            lastBlockNumber = result.line - 1;
            break;
        }
    }

    QTextDocument *doc = highlighter->document();
    QTC_ASSERT(lastBlockNumber + 1 < doc->blockCount(), return);
    QTextBlock b = doc->findBlockByNumber(lastBlockNumber + 1);

    while (b.isValid()) {
        highlighter->setExtraAdditionalFormats(b, QList<QTextLayout::FormatRange>());
        b = b.next();
    }
}
