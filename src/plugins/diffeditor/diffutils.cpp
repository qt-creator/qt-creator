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
#include "texteditor/fontsettings.h"

namespace DiffEditor {
namespace Internal {

static QList<TextLineData> assemblyRows(const QList<TextLineData> &lines,
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

static bool lastLinesEqual(const QList<TextLineData> &leftLines,
                           const QList<TextLineData> &rightLines)
{
    const bool leftLineEqual = leftLines.count()
            ? leftLines.last().text.isEmpty()
            : true;
    const bool rightLineEqual = rightLines.count()
            ? rightLines.last().text.isEmpty()
            : true;
    return leftLineEqual && rightLineEqual;
}

static void handleLine(const QStringList &newLines,
                       int line,
                       QList<TextLineData> *lines,
                       int *lineNumber)
{
    if (line < newLines.count()) {
        const QString text = newLines.at(line);
        if (lines->isEmpty() || line > 0) {
            if (line > 0)
                ++*lineNumber;
            lines->append(TextLineData(text));
        } else {
            lines->last().text += text;
        }
    }
}

static void handleDifference(const QString &text,
                             QList<TextLineData> *lines,
                             int *lineNumber)
{
    const QStringList newLines = text.split(QLatin1Char('\n'));
    for (int line = 0; line < newLines.count(); ++line) {
        const int startPos = line > 0
                ? -1
                : lines->isEmpty() ? 0 : lines->last().text.count();
        handleLine(newLines, line, lines, lineNumber);
        const int endPos = line < newLines.count() - 1
                ? -1
                : lines->isEmpty() ? 0 : lines->last().text.count();
        if (!lines->isEmpty())
            lines->last().changedPositions.insert(startPos, endPos);
    }
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

    QList<TextLineData> leftLines;
    QList<TextLineData> rightLines;

    // <line number, span count>
    QMap<int, int> leftSpans;
    QMap<int, int> rightSpans;
    // <left line number, right line number>
    QMap<int, int> equalLines;

    int leftLineNumber = 0;
    int rightLineNumber = 0;
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
            handleDifference(leftDiff.text, &leftLines, &leftLineNumber);
            lastLineEqual = lastLinesEqual(leftLines, rightLines);
            i++;
        }
        if (rightDiff.command == Diff::Insert) {
            // process insert
            handleDifference(rightDiff.text, &rightLines, &rightLineNumber);
            lastLineEqual = lastLinesEqual(leftLines, rightLines);
            j++;
        }
        if (leftDiff.command == Diff::Equal && rightDiff.command == Diff::Equal) {
            // process equal
            const QStringList newLeftLines = leftDiff.text.split(QLatin1Char('\n'));
            const QStringList newRightLines = rightDiff.text.split(QLatin1Char('\n'));

            int line = 0;

            while (line < qMax(newLeftLines.count(), newRightLines.count())) {
                handleLine(newLeftLines, line, &leftLines, &leftLineNumber);
                handleLine(newRightLines, line, &rightLines, &rightLineNumber);

                const int commonLineCount = qMin(newLeftLines.count(), newRightLines.count());
                if (line < commonLineCount) {
                    // try to align
                    const int leftDifference = leftLineNumber - leftLineAligned;
                    const int rightDifference = rightLineNumber - rightLineAligned;

                    if (leftDifference && rightDifference) {
                        bool doAlign = true;
                        if (line == 0 // omit alignment when first lines of equalities are empty and last generated lines are not equal
                                && (newLeftLines.at(0).isEmpty() || newRightLines.at(0).isEmpty())
                                && !lastLineEqual) {
                            doAlign = false;
                        }

                        if (line == commonLineCount - 1) {
                            // omit alignment when last lines of equalities are empty
                            if (leftLines.last().text.isEmpty() || rightLines.last().text.isEmpty())
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
        if (equalLines.value(leftLine, -2) == rightLine)
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
    while (i < originalData.rows.count()) {
        const bool contextChunk = hiddenRows.contains(i);
        ChunkData chunkData;
        chunkData.contextChunk = contextChunk;
        while (i < originalData.rows.count()) {
            if (contextChunk != hiddenRows.contains(i))
                break;
            RowData rowData = originalData.rows.at(i);
            chunkData.rows.append(rowData);
            ++i;
        }
        fileData.chunks.append(chunkData);
    }

    return fileData;
}

void addChangedPositions(int positionOffset, const QMap<int, int> &originalChangedPositions, QMap<int, int> *changedPositions)
{
    QMapIterator<int, int> it(originalChangedPositions);
    while (it.hasNext()) {
        it.next();
        const int startPos = it.key();
        const int endPos = it.value();
        const int newStartPos = startPos < 0 ? -1 : startPos + positionOffset;
        const int newEndPos = endPos < 0 ? -1 : endPos + positionOffset;
        if (startPos < 0 && !changedPositions->isEmpty())
            changedPositions->insert(changedPositions->lastKey(), newEndPos);
        else
            changedPositions->insert(newStartPos, newEndPos);
    }
}

QList<QTextEdit::ExtraSelection> colorPositions(
        const QTextCharFormat &format,
        QTextCursor &cursor,
        const QMap<int, int> &positions)
{
    QList<QTextEdit::ExtraSelection> lineSelections;

    cursor.setPosition(0);
    QMapIterator<int, int> itPositions(positions);
    while (itPositions.hasNext()) {
        itPositions.next();

        cursor.setPosition(itPositions.key());
        cursor.setPosition(itPositions.value(), QTextCursor::KeepAnchor);

        QTextEdit::ExtraSelection selection;
        selection.cursor = cursor;
        selection.format = format;
        lineSelections.append(selection);
    }
    return lineSelections;
}

QTextCharFormat fullWidthFormatForTextStyle(const TextEditor::FontSettings &fontSettings,
                                            TextEditor::TextStyle textStyle)
{
    QTextCharFormat format = fontSettings.toTextCharFormat(textStyle);
    format.setProperty(QTextFormat::FullWidthSelection, true);
    return format;
}

} // namespace Internal
} // namespace DiffEditor
