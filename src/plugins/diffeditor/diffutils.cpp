/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "diffutils.h"
#include "differ.h"
#include <QStringList>

namespace DiffEditor {
namespace Internal {

static QList<TextLineData> assemblyRows(const QStringList &lines,
                                        const QMap<int, int> &lineSpans)
{
    QList<TextLineData> data;

    const int lineCount = lines.count();
    for (int i = 0; i <= lineCount; i++) {
        for (int j = 0; j < lineSpans.value(i); j++)
            data.append(TextLineData(TextLineData::Separator));
        if (i < lineCount)
            data.append(lines.at(i));
    }
    return data;
}

static bool lastLinesEqual(const QStringList &leftLines,
                           const QStringList &rightLines)
{
    const bool leftLineEqual = leftLines.count()
            ? leftLines.last().isEmpty()
            : true;
    const bool rightLineEqual = rightLines.count()
            ? rightLines.last().isEmpty()
            : true;
    return leftLineEqual && rightLineEqual;
}

static void handleLine(const QStringList &newLines,
                       int line,
                       QStringList *lines,
                       int *lineNumber,
                       int *charNumber)
{
    if (line < newLines.count()) {
        const QString text = newLines.at(line);
        if (lines->isEmpty() || line > 0) {
            if (line > 0)
                ++*lineNumber;
            lines->append(text);
        } else {
            lines->last() += text;
        }
        *charNumber += text.count();
    }
}

static void handleDifference(const QString &text,
                             QStringList *lines,
                             QMap<int, int> *changedPositions,
                             int *lineNumber,
                             int *charNumber)
{
    const int oldPosition = *lineNumber + *charNumber;
    const QStringList newLeftLines = text.split(QLatin1Char('\n'));
    for (int line = 0; line < newLeftLines.count(); ++line)
        handleLine(newLeftLines, line, lines, lineNumber, charNumber);
    const int newPosition = *lineNumber + *charNumber;
    changedPositions->insert(oldPosition, newPosition);
}

/*
 * leftDiffList can contain only deletions and equalities,
 * while rightDiffList can contain only insertions and equalities.
 * The number of equalities on both lists must be the same.
*/
ChunkData calculateOriginalData(const QList<Diff> &leftDiffList,
                                const QList<Diff> &rightDiffList)
{
    ChunkData chunkData;

    int i = 0;
    int j = 0;

    QStringList leftLines;
    QStringList rightLines;

    // <line number, span count>
    QMap<int, int> leftSpans;
    QMap<int, int> rightSpans;
    // <left line number, right line number>
    QMap<int, int> equalLines;

    int leftLineNumber = 0;
    int rightLineNumber = 0;
    int leftCharNumber = 0;
    int rightCharNumber = 0;
    int leftLineAligned = -1;
    int rightLineAligned = -1;
    bool lastLineEqual = true;

    while (i <= leftDiffList.count() && j <= rightDiffList.count()) {
        const Diff leftDiff = i < leftDiffList.count()
                ? leftDiffList.at(i)
                : Diff(Diff::Equal);
        const Diff rightDiff = j < rightDiffList.count()
                ? rightDiffList.at(j)
                : Diff(Diff::Equal);

        if (leftDiff.command == Diff::Delete) {
            // process delete
            handleDifference(leftDiff.text, &leftLines, &chunkData.changedLeftPositions, &leftLineNumber, &leftCharNumber);
            lastLineEqual = lastLinesEqual(leftLines, rightLines);
            i++;
        }
        if (rightDiff.command == Diff::Insert) {
            // process insert
            handleDifference(rightDiff.text, &rightLines, &chunkData.changedRightPositions, &rightLineNumber, &rightCharNumber);
            lastLineEqual = lastLinesEqual(leftLines, rightLines);
            j++;
        }
        if (leftDiff.command == Diff::Equal && rightDiff.command == Diff::Equal) {
            // process equal
            const QStringList newLeftLines = leftDiff.text.split(QLatin1Char('\n'));
            const QStringList newRightLines = rightDiff.text.split(QLatin1Char('\n'));

            int line = 0;

            while (line < qMax(newLeftLines.count(), newRightLines.count())) {
                handleLine(newLeftLines, line, &leftLines, &leftLineNumber, &leftCharNumber);
                handleLine(newRightLines, line, &rightLines, &rightLineNumber, &rightCharNumber);

                const int commonLineCount = qMin(newLeftLines.count(), newRightLines.count());
                if (line < commonLineCount) {
                    // try to align
                    const int leftDifference = leftLineNumber - leftLineAligned;
                    const int rightDifference = rightLineNumber - rightLineAligned;

                    if (leftDifference && rightDifference) {
                        bool doAlign = true;
                        if (line == 0 // omit alignment when first lines of equalities are empty
                                && (newLeftLines.at(0).isEmpty() || newRightLines.at(0).isEmpty())) {
                            doAlign = false;
                        }

                        if (line == commonLineCount - 1) {
                            // omit alignment when last lines of equalities are empty
                            if (leftLines.last().isEmpty() || rightLines.last().isEmpty())
                                doAlign = false;

                            // unless it's the last dummy line (don't omit in that case)
                            if (i == leftDiffList.count() && j == rightDiffList.count())
                                doAlign = true;
                        }

                        if (doAlign) {
                            // align here
                            leftLineAligned = leftLineNumber;
                            rightLineAligned = rightLineNumber;

                            // insert separators if needed
                            if (rightDifference > leftDifference)
                                leftSpans.insert(leftLineNumber, rightDifference - leftDifference);
                            else if (leftDifference > rightDifference)
                                rightSpans.insert(rightLineNumber, leftDifference - rightDifference);
                        }
                    }
                }

                // check if lines are equal
                if ((line < commonLineCount - 1) // before the last common line in equality
                        || (line == commonLineCount - 1 // or the last common line in equality
                            && i == leftDiffList.count() // and it's the last iteration
                            && j == rightDiffList.count())) {
                    if (line > 0 || lastLineEqual)
                        equalLines.insert(leftLineNumber, rightLineNumber);
                }

                if (line > 0)
                    lastLineEqual = true;

                line++;
            }
            i++;
            j++;
        }
    }

    QList<TextLineData> leftData = assemblyRows(leftLines,
                                                leftSpans);
    QList<TextLineData> rightData = assemblyRows(rightLines,
                                                 rightSpans);

    // fill ending separators
    for (int i = leftData.count(); i < rightData.count(); i++)
        leftData.append(TextLineData(TextLineData::Separator));
    for (int i = rightData.count(); i < leftData.count(); i++)
        rightData.append(TextLineData(TextLineData::Separator));

    const int visualLineCount = leftData.count();
    int leftLine = -1;
    int rightLine = -1;
    for (int i = 0; i < visualLineCount; i++) {
        const TextLineData &leftTextLine = leftData.at(i);
        const TextLineData &rightTextLine = rightData.at(i);
        RowData row(leftTextLine, rightTextLine);

        if (leftTextLine.textLineType == TextLineData::TextLine)
            ++leftLine;
        if (rightTextLine.textLineType == TextLineData::TextLine)
            ++rightLine;
        if (equalLines.value(leftLine, -1) == rightLine)
            row.equal = true;

        chunkData.rows.append(row);
    }
    return chunkData;
}

FileData calculateContextData(const ChunkData &originalData, int contextLinesNumber)
{
    if (contextLinesNumber < 0)
        return FileData(originalData);

    const int joinChunkThreshold = 1;

    FileData fileData;
    QMap<int, bool> hiddenRows;
    int i = 0;
    while (i < originalData.rows.count()) {
        const RowData &row = originalData.rows[i];
        if (row.equal) {
            // count how many equal
            int equalRowStart = i;
            i++;
            while (i < originalData.rows.count()) {
                const RowData originalRow = originalData.rows.at(i);
                if (!originalRow.equal)
                    break;
                i++;
            }
            const bool first = equalRowStart == 0; // includes first line?
            const bool last = i == originalData.rows.count(); // includes last line?

            const int firstLine = first ? 0 : equalRowStart + contextLinesNumber;
            const int lastLine = last ? originalData.rows.count() : i - contextLinesNumber;

            if (firstLine < lastLine - joinChunkThreshold) {
                for (int j = firstLine; j < lastLine; j++) {
                    hiddenRows.insert(j, true);
                }
            }
        } else {
            // iterate to the next row
            i++;
        }
    }
    i = 0;
    int leftCharCounter = 0;
    int rightCharCounter = 0;
    QMap<int, int>::ConstIterator leftChangedIt = originalData.changedLeftPositions.constBegin();
    QMap<int, int>::ConstIterator rightChangedIt = originalData.changedRightPositions.constBegin();
    const QMap<int, int>::ConstIterator leftChangedItEnd = originalData.changedLeftPositions.constEnd();
    const QMap<int, int>::ConstIterator rightChangedItEnd = originalData.changedRightPositions.constEnd();
    while (i < originalData.rows.count()) {
        if (!hiddenRows.contains(i)) {
            ChunkData chunkData;
            int leftOffset = leftCharCounter;
            int rightOffset = rightCharCounter;
            chunkData.contextChunk = false;
            while (i < originalData.rows.count()) {
                if (hiddenRows.contains(i))
                    break;
                RowData rowData = originalData.rows.at(i);
                chunkData.rows.append(rowData);

                if (rowData.leftLine.textLineType == TextLineData::TextLine)
                    leftCharCounter += rowData.leftLine.text.count() + 1; // +1 for '\n'
                if (rowData.rightLine.textLineType == TextLineData::TextLine)
                    rightCharCounter += rowData.rightLine.text.count() + 1; // +1 for '\n'
                i++;
            }
            while (leftChangedIt != leftChangedItEnd) {
                if (leftChangedIt.key() < leftOffset
                        || leftChangedIt.key() > leftCharCounter)
                    break;

                const int startPos = leftChangedIt.key();
                const int endPos = leftChangedIt.value();
                chunkData.changedLeftPositions.insert(startPos - leftOffset, endPos - leftOffset);
                leftChangedIt++;
            }
            while (rightChangedIt != rightChangedItEnd) {
                if (rightChangedIt.key() < rightOffset
                        || rightChangedIt.key() > rightCharCounter)
                    break;

                const int startPos = rightChangedIt.key();
                const int endPos = rightChangedIt.value();
                chunkData.changedRightPositions.insert(startPos - rightOffset, endPos - rightOffset);
                rightChangedIt++;
            }
            fileData.chunks.append(chunkData);
        } else {
            ChunkData chunkData;
            chunkData.contextChunk = true;
            while (i < originalData.rows.count()) {
                if (!hiddenRows.contains(i))
                    break;
                RowData rowData = originalData.rows.at(i);
                chunkData.rows.append(rowData);

                if (rowData.leftLine.textLineType == TextLineData::TextLine)
                    leftCharCounter += rowData.leftLine.text.count() + 1; // +1 for '\n'
                if (rowData.rightLine.textLineType == TextLineData::TextLine)
                    rightCharCounter += rowData.rightLine.text.count() + 1; // +1 for '\n'
                i++;
            }
            fileData.chunks.append(chunkData);
        }
    }
    return fileData;
}

} // namespace Internal
} // namespace DiffEditor
