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

#include "diffutils.h"
#include "differ.h"
#include <QStringList>
#include <QTextStream>
#include "texteditor/fontsettings.h"

namespace DiffEditor {

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
ChunkData DiffUtils::calculateOriginalData(const QList<Diff> &leftDiffList,
                                const QList<Diff> &rightDiffList)
{
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
            if (j == rightDiffList.count() && lastLineEqual && leftDiff.text.startsWith(QLatin1Char('\n')))
                equalLines.insert(leftLineNumber, rightLineNumber);
            // process delete
            handleDifference(leftDiff.text, &leftLines, &leftLineNumber);
            lastLineEqual = lastLinesEqual(leftLines, rightLines);
            if (j == rightDiffList.count())
                lastLineEqual = false;
            i++;
        }
        if (rightDiff.command == Diff::Insert) {
            if (i == leftDiffList.count() && lastLineEqual && rightDiff.text.startsWith(QLatin1Char('\n')))
                equalLines.insert(leftLineNumber, rightLineNumber);
            // process insert
            handleDifference(rightDiff.text, &rightLines, &rightLineNumber);
            lastLineEqual = lastLinesEqual(leftLines, rightLines);
            if (i == leftDiffList.count())
                lastLineEqual = false;
            j++;
        }
        if (leftDiff.command == Diff::Equal && rightDiff.command == Diff::Equal) {
            // process equal
            const QStringList newLeftLines = leftDiff.text.split(QLatin1Char('\n'));
            const QStringList newRightLines = rightDiff.text.split(QLatin1Char('\n'));

            int line = 0;

            if (i < leftDiffList.count() || j < rightDiffList.count() || (leftLines.count() && rightLines.count())) {
                while (line < qMax(newLeftLines.count(), newRightLines.count())) {
                    handleLine(newLeftLines, line, &leftLines, &leftLineNumber);
                    handleLine(newRightLines, line, &rightLines, &rightLineNumber);

                    const int commonLineCount = qMin(newLeftLines.count(),
                                                     newRightLines.count());
                    if (line < commonLineCount) {
                        // try to align
                        const int leftDifference = leftLineNumber - leftLineAligned;
                        const int rightDifference = rightLineNumber - rightLineAligned;

                        if (leftDifference && rightDifference) {
                            bool doAlign = true;
                            if (line == 0
                                    && (newLeftLines.at(0).isEmpty()
                                        || newRightLines.at(0).isEmpty())
                                    && !lastLineEqual) {
                                // omit alignment when first lines of equalities
                                // are empty and last generated lines are not equal
                                doAlign = false;
                            }

                            if (line == commonLineCount - 1) {
                                // omit alignment when last lines of equalities are empty
                                if (leftLines.last().text.isEmpty()
                                        || rightLines.last().text.isEmpty())
                                    doAlign = false;

                                // unless it's the last dummy line (don't omit in that case)
                                if (i == leftDiffList.count()
                                        && j == rightDiffList.count())
                                    doAlign = true;
                            }

                            if (doAlign) {
                                // align here
                                leftLineAligned = leftLineNumber;
                                rightLineAligned = rightLineNumber;

                                // insert separators if needed
                                if (rightDifference > leftDifference)
                                    leftSpans.insert(leftLineNumber,
                                                     rightDifference - leftDifference);
                                else if (leftDifference > rightDifference)
                                    rightSpans.insert(rightLineNumber,
                                                      leftDifference - rightDifference);
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
    ChunkData chunkData;

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

FileData DiffUtils::calculateContextData(const ChunkData &originalData, int contextLineCount,
                                         int joinChunkThreshold)
{
    if (contextLineCount < 0)
        return FileData(originalData);

    FileData fileData;
    fileData.contextChunksIncluded = true;
    fileData.lastChunkAtTheEndOfFile = true;

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

            const int firstLine = first
                    ? 0 : equalRowStart + contextLineCount;
            const int lastLine = last
                    ? originalData.rows.count() : i - contextLineCount;

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
    int leftLineNumber = 0;
    int rightLineNumber = 0;
    while (i < originalData.rows.count()) {
        const bool contextChunk = hiddenRows.contains(i);
        ChunkData chunkData;
        chunkData.contextChunk = contextChunk;
        chunkData.leftStartingLineNumber = leftLineNumber;
        chunkData.rightStartingLineNumber = rightLineNumber;
        while (i < originalData.rows.count()) {
            if (contextChunk != hiddenRows.contains(i))
                break;
            RowData rowData = originalData.rows.at(i);
            chunkData.rows.append(rowData);
            if (rowData.leftLine.textLineType == TextLineData::TextLine)
                ++leftLineNumber;
            if (rowData.rightLine.textLineType == TextLineData::TextLine)
                ++rightLineNumber;
            ++i;
        }
        fileData.chunks.append(chunkData);
    }

    return fileData;
}

QString DiffUtils::makePatchLine(const QChar &startLineCharacter,
                          const QString &textLine,
                          bool lastChunk,
                          bool lastLine)
{
    QString line;

    const bool addNoNewline = lastChunk // it's the last chunk in file
            && lastLine // it's the last row in chunk
            && !textLine.isEmpty(); // the row is not empty

    const bool addLine = !lastChunk // not the last chunk in file
            || !lastLine // not the last row in chunk
            || addNoNewline; // no addNoNewline case

    if (addLine) {
        line = startLineCharacter + textLine + QLatin1Char('\n');
        if (addNoNewline)
            line += QLatin1String("\\ No newline at end of file\n");
    }

    return line;
}

QString DiffUtils::makePatch(const ChunkData &chunkData,
                             bool lastChunk)
{
    if (chunkData.contextChunk)
        return QString();

    QString diffText;
    int leftLineCount = 0;
    int rightLineCount = 0;
    QList<TextLineData> leftBuffer, rightBuffer;

    int rowToBeSplit = -1;

    if (lastChunk) {
        // Detect the case when the last equal line is followed by
        // only separators on left or on right. In that case
        // the last equal line needs to be split.
        const int rowCount = chunkData.rows.count();
        int i = 0;
        for (i = rowCount; i > 0; i--) {
            const RowData &rowData = chunkData.rows.at(i - 1);
            if (rowData.leftLine.textLineType != TextLineData::Separator
                    || rowData.rightLine.textLineType != TextLineData::TextLine)
                break;
        }
        const int leftSeparator = i;
        for (i = rowCount; i > 0; i--) {
            const RowData &rowData = chunkData.rows.at(i - 1);
            if (rowData.rightLine.textLineType != TextLineData::Separator
                    || rowData.leftLine.textLineType != TextLineData::TextLine)
                break;
        }
        const int rightSeparator = i;
        const int commonSeparator = qMin(leftSeparator, rightSeparator);
        if (commonSeparator > 0
                && commonSeparator < rowCount
                && chunkData.rows.at(commonSeparator - 1).equal)
            rowToBeSplit = commonSeparator - 1;
    }

    for (int i = 0; i <= chunkData.rows.count(); i++) {
        const RowData &rowData = i < chunkData.rows.count()
                ? chunkData.rows.at(i)
                : RowData(TextLineData(TextLineData::Separator)); // dummy,
                                        // ensure we process buffers to the end.
                                        // rowData will be equal
        if (rowData.equal && i != rowToBeSplit) {
            if (leftBuffer.count()) {
                for (int j = 0; j < leftBuffer.count(); j++) {
                    const QString line = makePatchLine(QLatin1Char('-'),
                                              leftBuffer.at(j).text,
                                              lastChunk,
                                              i == chunkData.rows.count()
                                                       && j == leftBuffer.count() - 1);

                    if (!line.isEmpty())
                        ++leftLineCount;

                    diffText += line;
                }
                leftBuffer.clear();
            }
            if (rightBuffer.count()) {
                for (int j = 0; j < rightBuffer.count(); j++) {
                    const QString line = makePatchLine(QLatin1Char('+'),
                                              rightBuffer.at(j).text,
                                              lastChunk,
                                              i == chunkData.rows.count()
                                                       && j == rightBuffer.count() - 1);

                    if (!line.isEmpty())
                        ++rightLineCount;

                    diffText += line;
                }
                rightBuffer.clear();
            }
            if (i < chunkData.rows.count()) {
                const QString line = makePatchLine(QLatin1Char(' '),
                                          rowData.rightLine.text,
                                          lastChunk,
                                          i == chunkData.rows.count() - 1);

                if (!line.isEmpty()) {
                    ++leftLineCount;
                    ++rightLineCount;
                }

                diffText += line;
            }
        } else {
            if (rowData.leftLine.textLineType == TextLineData::TextLine)
                leftBuffer.append(rowData.leftLine);
            if (rowData.rightLine.textLineType == TextLineData::TextLine)
                rightBuffer.append(rowData.rightLine);
        }
    }

    const QString chunkLine = QLatin1String("@@ -")
            + QString::number(chunkData.leftStartingLineNumber + 1)
            + QLatin1Char(',')
            + QString::number(leftLineCount)
            + QLatin1String(" +")
            + QString::number(chunkData.rightStartingLineNumber + 1)
            + QLatin1Char(',')
            + QString::number(rightLineCount)
            + QLatin1String(" @@")
            + chunkData.contextInfo
            + QLatin1Char('\n');

    diffText.prepend(chunkLine);

    return diffText;
}

QString DiffUtils::makePatch(const ChunkData &chunkData,
                             const QString &leftFileName,
                             const QString &rightFileName,
                             bool lastChunk)
{
    QString diffText = makePatch(chunkData, lastChunk);

    const QString rightFileInfo = QLatin1String("+++ ") + rightFileName + QLatin1Char('\n');
    const QString leftFileInfo = QLatin1String("--- ") + leftFileName + QLatin1Char('\n');

    diffText.prepend(rightFileInfo);
    diffText.prepend(leftFileInfo);

    return diffText;
}

QString DiffUtils::makePatch(const QList<FileData> &fileDataList, unsigned formatFlags)
{
    QString diffText;
    QTextStream str(&diffText);

    for (int i = 0; i < fileDataList.count(); i++) {
        const FileData &fileData = fileDataList.at(i);
        if (formatFlags & GitFormat) {
            str << "diff --git a/" << fileData.leftFileInfo.fileName
                << " b/" << fileData.rightFileInfo.fileName << '\n';
        }
        if (fileData.binaryFiles) {
            str << "Binary files ";
            if (formatFlags & AddLevel)
                str << "a/";
            str << fileData.leftFileInfo.fileName << " and ";
            if (formatFlags & AddLevel)
                str << "b/";
            str << fileData.rightFileInfo.fileName << " differ\n";
        } else {
            str << "--- ";
            if (formatFlags & AddLevel)
                str << "a/";
            str << fileData.leftFileInfo.fileName << "\n+++ ";
            if (formatFlags & AddLevel)
                str << "b/";
            str << fileData.rightFileInfo.fileName << '\n';
            for (int j = 0; j < fileData.chunks.count(); j++) {
                str << makePatch(fileData.chunks.at(j),
                                      (j == fileData.chunks.count() - 1)
                                      && fileData.lastChunkAtTheEndOfFile);
            }
        }
    }
    return diffText;
}

static QList<RowData> readLines(const QString &patch,
                                bool lastChunk,
                                bool *lastChunkAtTheEndOfFile,
                                bool *ok)
{
//    const QRegExp lineRegExp(QLatin1String("(?:\\n)"                // beginning of the line
//                                           "([ -\\+\\\\])([^\\n]*)" // -, +, \\ or space, followed by any no-newline character
//                                           "(?:\\n|$)"));           // end of line or file
    QList<Diff> diffList;

    const QChar newLine = QLatin1Char('\n');

    int lastEqual = -1;
    int lastDelete = -1;
    int lastInsert = -1;

    int noNewLineInEqual = -1;
    int noNewLineInDelete = -1;
    int noNewLineInInsert = -1;

    const QStringList lines = patch.split(newLine);
    int i;
    for (i = 0; i < lines.count(); i++) {
        const QString line = lines.at(i);
        if (line.isEmpty()) { // need to have at least one character (1 column)
            if (lastChunk)
                i = lines.count(); // pretend as we've read all the lines (we just ignore the rest)
            break;
        }
        QChar firstCharacter = line.at(0);
        if (firstCharacter == QLatin1Char('\\')) { // no new line marker
            if (!lastChunk) // can only appear in last chunk of the file
                break;
            if (!diffList.isEmpty()) {
                Diff &last = diffList.last();
                if (last.text.isEmpty())
                    break;

                if (last.command == Diff::Equal) {
                    if (noNewLineInEqual >= 0)
                        break;
                    noNewLineInEqual = diffList.count() - 1;
                } else if (last.command == Diff::Delete) {
                    if (noNewLineInDelete >= 0)
                        break;
                    noNewLineInDelete = diffList.count() - 1;
                } else if (last.command == Diff::Insert) {
                    if (noNewLineInInsert >= 0)
                        break;
                    noNewLineInInsert = diffList.count() - 1;
                }
            }
        } else {
            Diff::Command command = Diff::Equal;
            if (firstCharacter == QLatin1Char(' ')) { // common line
                command = Diff::Equal;
            } else if (firstCharacter == QLatin1Char('-')) { // deleted line
                command = Diff::Delete;
            } else if (firstCharacter == QLatin1Char('+')) { // inserted line
                command = Diff::Insert;
            } else { // no other character may exist as the first character
                if (lastChunk)
                    i = lines.count(); // pretend as we've read all the lines (we just ignore the rest)
                break;
            }

            Diff diffToBeAdded(command, line.mid(1) + newLine);

            if (!diffList.isEmpty() && diffList.last().command == command)
                diffList.last().text.append(diffToBeAdded.text);
            else
                diffList.append(diffToBeAdded);

            if (command == Diff::Equal) // common line
                lastEqual = diffList.count() - 1;
            else if (command == Diff::Delete) // deleted line
                lastDelete = diffList.count() - 1;
            else if (command == Diff::Insert) // inserted line
                lastInsert = diffList.count() - 1;
        }
    }

    if (i < lines.count() // we broke before
            // or we have noNewLine in some equal line and in either delete or insert line
            || (noNewLineInEqual >= 0 && (noNewLineInDelete >= 0 || noNewLineInInsert >= 0))
            // or we have noNewLine in not the last equal line
            || (noNewLineInEqual >= 0 && noNewLineInEqual != lastEqual)
            // or we have noNewLine in not the last delete line or there is a equal line after the noNewLine for delete
            || (noNewLineInDelete >= 0 && (noNewLineInDelete != lastDelete || lastEqual > lastDelete))
            // or we have noNewLine in not the last insert line or there is a equal line after the noNewLine for insert
            || (noNewLineInInsert >= 0 && (noNewLineInInsert != lastInsert || lastEqual > lastInsert))) {
        if (ok)
            *ok = false;
        return QList<RowData>();
    }

    if (ok)
        *ok = true;

    bool removeNewLineFromLastEqual = false;
    bool removeNewLineFromLastDelete = false;
    bool removeNewLineFromLastInsert = false;
    bool prependNewLineAfterLastEqual = false;

    if (noNewLineInDelete >= 0 || noNewLineInInsert >= 0) {
        if (noNewLineInDelete >= 0)
            removeNewLineFromLastDelete = true;
        if (noNewLineInInsert >= 0)
            removeNewLineFromLastInsert = true;
    } else {
        if (noNewLineInEqual >= 0) {
            removeNewLineFromLastEqual = true;
        } else {
            if (lastEqual > lastDelete && lastEqual > lastInsert) {
                removeNewLineFromLastEqual = true;
            } else if (lastDelete > lastEqual && lastDelete > lastInsert) {
                if (lastInsert > lastEqual) {
                    removeNewLineFromLastDelete = true;
                    removeNewLineFromLastInsert = true;
                } else if (lastEqual > lastInsert) {
                    removeNewLineFromLastEqual = true;
                    removeNewLineFromLastDelete = true;
                    prependNewLineAfterLastEqual = true;
                }
            } else if (lastInsert > lastEqual && lastInsert > lastDelete) {
                if (lastDelete > lastEqual) {
                    removeNewLineFromLastDelete = true;
                    removeNewLineFromLastInsert = true;
                } else if (lastEqual > lastDelete) {
                    removeNewLineFromLastEqual = true;
                    removeNewLineFromLastInsert = true;
                    prependNewLineAfterLastEqual = true;
                }
            }
        }
    }


    if (removeNewLineFromLastEqual) {
        Diff &diff = diffList[lastEqual];
        diff.text = diff.text.left(diff.text.count() - 1);
    }
    if (removeNewLineFromLastDelete) {
        Diff &diff = diffList[lastDelete];
        diff.text = diff.text.left(diff.text.count() - 1);
    }
    if (removeNewLineFromLastInsert) {
        Diff &diff = diffList[lastInsert];
        diff.text = diff.text.left(diff.text.count() - 1);
    }
    if (prependNewLineAfterLastEqual) {
        Diff &diff = diffList[lastEqual + 1];
        diff.text = newLine + diff.text;
    }

    if (lastChunkAtTheEndOfFile) {
        *lastChunkAtTheEndOfFile = noNewLineInEqual >= 0
                || noNewLineInDelete >= 0|| noNewLineInInsert >= 0;
    }

//    diffList = Differ::merge(diffList);
    QList<Diff> leftDiffList;
    QList<Diff> rightDiffList;
    Differ::splitDiffList(diffList, &leftDiffList, &rightDiffList);
    QList<Diff> outputLeftDiffList;
    QList<Diff> outputRightDiffList;

    Differ::diffBetweenEqualities(leftDiffList,
                                  rightDiffList,
                                  &outputLeftDiffList,
                                  &outputRightDiffList);

    return DiffUtils::calculateOriginalData(outputLeftDiffList,
                                            outputRightDiffList).rows;
}

static QList<ChunkData> readChunks(const QString &patch,
                                   bool *lastChunkAtTheEndOfFile,
                                   bool *ok)
{
    const QRegExp chunkRegExp(QLatin1String(
                                  // beginning of the line
                              "(?:\\n|^)"
                                  // @@ -leftPos[,leftCount] +rightPos[,rightCount] @@
                              "@@ -(\\d+)(?:,\\d+)? \\+(\\d+)(?:,\\d+)? @@"
                                  // optional hint (e.g. function name)
                              "(\\ +[^\\n]*)?"
                                  // end of line (need to be followed by text line)
                              "\\n"));

    bool readOk = false;

    QList<ChunkData> chunkDataList;

    int pos = chunkRegExp.indexIn(patch);
    if (pos == 0) {
        int endOfLastChunk = 0;
        do {
            const int leftStartingPos = chunkRegExp.cap(1).toInt();
            const int rightStartingPos = chunkRegExp.cap(2).toInt();
            const QString contextInfo = chunkRegExp.cap(3);
            if (endOfLastChunk > 0) {
                const QString lines = patch.mid(endOfLastChunk,
                                                pos - endOfLastChunk);
                chunkDataList.last().rows = readLines(lines,
                                                      false,
                                                      lastChunkAtTheEndOfFile,
                                                      &readOk);
                if (!readOk)
                    break;
            }
            pos += chunkRegExp.matchedLength();
            endOfLastChunk = pos;
            ChunkData chunkData;
            chunkData.leftStartingLineNumber = leftStartingPos - 1;
            chunkData.rightStartingLineNumber = rightStartingPos - 1;
            chunkData.contextInfo = contextInfo;
            chunkDataList.append(chunkData);
        } while ((pos = chunkRegExp.indexIn(patch, pos, QRegExp::CaretAtOffset)) != -1);

        if (endOfLastChunk > 0) {
            const QString lines = patch.mid(endOfLastChunk);
            chunkDataList.last().rows = readLines(lines,
                                                  true,
                                                  lastChunkAtTheEndOfFile,
                                                  &readOk);
        }
    }

    if (ok)
        *ok = readOk;

    return chunkDataList;
}

static FileData readDiffHeaderAndChunks(const QString &headerAndChunks,
                                        bool *ok)
{
    QString patch = headerAndChunks;
    FileData fileData;
    bool readOk = false;

    const QRegExp leftFileRegExp(QLatin1String(
                                     "(?:\\n|^)-{3} "        // "--- "
                                     "([^\\t\\n]+)"          // "fileName1"
                                     "(?:\\t[^\\n]*)*\\n")); // optionally followed by: \t anything \t anything ...)
    const QRegExp rightFileRegExp(QLatin1String(
                                      "^\\+{3} "              // "+++ "
                                      "([^\\t\\n]+)"          // "fileName2"
                                      "(?:\\t[^\\n]*)*\\n")); // optionally followed by: \t anything \t anything ...)
    const QRegExp binaryRegExp(QLatin1String("^Binary files ([^\\t\\n]+) and ([^\\t\\n]+) differ$"));

    // followed either by leftFileRegExp or by binaryRegExp
    if (leftFileRegExp.indexIn(patch) == 0) {
        patch.remove(0, leftFileRegExp.matchedLength());
        fileData.leftFileInfo.fileName = leftFileRegExp.cap(1);

        // followed by rightFileRegExp
        if (rightFileRegExp.indexIn(patch) == 0) {
            patch.remove(0, rightFileRegExp.matchedLength());
            fileData.rightFileInfo.fileName = rightFileRegExp.cap(1);

            fileData.chunks = readChunks(patch,
                                         &fileData.lastChunkAtTheEndOfFile,
                                         &readOk);
        }
    } else if (binaryRegExp.indexIn(patch) == 0) {
        fileData.leftFileInfo.fileName = binaryRegExp.cap(1);
        fileData.rightFileInfo.fileName = binaryRegExp.cap(2);
        fileData.binaryFiles = true;
        readOk = true;
    }

    if (ok)
        *ok = readOk;

    if (!readOk)
        return FileData();

    return fileData;

}

static QList<FileData> readDiffPatch(const QString &patch,
                                     bool *ok)
{
    const QRegExp diffRegExp(QLatin1String("(?:\\n|^)"          // new line of the beginning of a patch
                                           "("                  // either
                                           "-{3} "              // ---
                                           "[^\\t\\n]+"         // filename1
                                           "(?:\\t[^\\n]*)*\\n" // optionally followed by: \t anything \t anything ...
                                           "\\+{3} "            // +++
                                           "[^\\t\\n]+"         // filename2
                                           "(?:\\t[^\\n]*)*\\n" // optionally followed by: \t anything \t anything ...
                                           "|"                  // or
                                           "Binary files "
                                           "[^\\t\\n]+"         // filename1
                                           " and "
                                           "[^\\t\\n]+"         // filename2
                                           " differ"
                                           ")"));               // end of or

    bool readOk = false;

    QList<FileData> fileDataList;

    int pos = diffRegExp.indexIn(patch);
    if (pos >= 0) { // git style patch
        readOk = true;
        int lastPos = -1;
        do {
            if (lastPos >= 0) {
                const QString headerAndChunks = patch.mid(lastPos,
                                                          pos - lastPos);

                const FileData fileData = readDiffHeaderAndChunks(headerAndChunks,
                                                                  &readOk);

                if (!readOk)
                    break;

                fileDataList.append(fileData);
            }
            lastPos = pos;
            pos += diffRegExp.matchedLength();
        } while ((pos = diffRegExp.indexIn(patch, pos)) != -1);

        if (lastPos >= 0 && readOk) {
            const QString headerAndChunks = patch.mid(lastPos,
                                                      patch.count() - lastPos - 1);

            const FileData fileData = readDiffHeaderAndChunks(headerAndChunks,
                                                              &readOk);

            if (readOk)
                fileDataList.append(fileData);
        }
    }

    if (ok)
        *ok = readOk;

    if (!readOk)
        return QList<FileData>();

    return fileDataList;
}

static bool fileNameEnd(const QChar &c)
{
    return c == QLatin1Char('\n') || c == QLatin1Char('\t');
}

static FileData readGitHeaderAndChunks(const QString &headerAndChunks,
                                       const QString &fileName,
                                       bool *ok)
{
    FileData fileData;
    fileData.leftFileInfo.fileName = fileName;
    fileData.rightFileInfo.fileName = fileName;

    QString patch = headerAndChunks;
    bool readOk = false;

    const QString devNull(QLatin1String("/dev/null"));

    // will be followed by: index 0000000..shasha, file "a" replaced by "/dev/null", @@ -0,0 +m,n @@
    const QRegExp newFileMode(QLatin1String("^new file mode \\d+\\n")); // new file mode octal

    // will be followed by: index shasha..0000000, file "b" replaced by "/dev/null", @@ -m,n +0,0 @@
    const QRegExp deletedFileMode(QLatin1String("^deleted file mode \\d+\\n")); // deleted file mode octal

    const QRegExp modeChangeRegExp(QLatin1String("^old mode \\d+\\nnew mode \\d+\\n"));

    const QRegExp indexRegExp(QLatin1String("^index (\\w+)\\.{2}(\\w+)(?: \\d+)?(\\n|$)")); // index cap1..cap2(optionally: octal)

    QString leftFileName = QLatin1String("a/") + fileName;
    QString rightFileName = QLatin1String("b/") + fileName;

    if (newFileMode.indexIn(patch) == 0) {
        fileData.fileOperation = FileData::NewFile;
        leftFileName = devNull;
        patch.remove(0, newFileMode.matchedLength());
    } else if (deletedFileMode.indexIn(patch) == 0) {
        fileData.fileOperation = FileData::DeleteFile;
        rightFileName = devNull;
        patch.remove(0, deletedFileMode.matchedLength());
    } else if (modeChangeRegExp.indexIn(patch) == 0) {
        patch.remove(0, modeChangeRegExp.matchedLength());
    }

    if (indexRegExp.indexIn(patch) == 0) {
        fileData.leftFileInfo.typeInfo = indexRegExp.cap(1);
        fileData.rightFileInfo.typeInfo = indexRegExp.cap(2);

        patch.remove(0, indexRegExp.matchedLength());
    }

    const QString binaryLine = QString::fromLatin1("Binary files ") + leftFileName
            + QLatin1String(" and ") + rightFileName + QLatin1String(" differ");
    const QString leftStart = QString::fromLatin1("--- ") + leftFileName;
    QChar leftFollow = patch.count() > leftStart.count() ? patch.at(leftStart.count()) : QLatin1Char('\n');

    // empty or followed either by leftFileRegExp or by binaryRegExp
    if (patch.isEmpty() && (fileData.fileOperation == FileData::NewFile
                         || fileData.fileOperation == FileData::DeleteFile)) {
        readOk = true;
    } else if (patch.startsWith(leftStart) && fileNameEnd(leftFollow)) {
        patch.remove(0, patch.indexOf(QLatin1Char('\n'), leftStart.count()) + 1);

        const QString rightStart = QString::fromLatin1("+++ ") + rightFileName;
        QChar rightFollow = patch.count() > rightStart.count() ? patch.at(rightStart.count()) : QLatin1Char('\n');

        // followed by rightFileRegExp
        if (patch.startsWith(rightStart) && fileNameEnd(rightFollow)) {
            patch.remove(0, patch.indexOf(QLatin1Char('\n'), rightStart.count()) + 1);

            fileData.chunks = readChunks(patch,
                                         &fileData.lastChunkAtTheEndOfFile,
                                         &readOk);
        }
    } else if (patch == binaryLine) {
        readOk = true;
        fileData.binaryFiles = true;
    }

    if (ok)
        *ok = readOk;

    if (!readOk)
        return FileData();

    return fileData;
}

static FileData readCopyRenameChunks(const QString &copyRenameChunks,
                                     FileData::FileOperation fileOperation,
                                     const QString &leftFileName,
                                     const QString &rightFileName,
                                     bool *ok)
{
    FileData fileData;
    fileData.fileOperation = fileOperation;
    fileData.leftFileInfo.fileName = leftFileName;
    fileData.rightFileInfo.fileName = rightFileName;

    QString patch = copyRenameChunks;
    bool readOk = false;

    const QRegExp indexRegExp(QLatin1String("^index (\\w+)\\.{2}(\\w+)(?: \\d+)?(\\n|$)")); // index cap1..cap2(optionally: octal)

    if (fileOperation == FileData::CopyFile || fileOperation == FileData::RenameFile) {
        if (indexRegExp.indexIn(patch) == 0) {
            fileData.leftFileInfo.typeInfo = indexRegExp.cap(1);
            fileData.rightFileInfo.typeInfo = indexRegExp.cap(2);

            patch.remove(0, indexRegExp.matchedLength());

            const QString leftStart = QString::fromLatin1("--- a/") + leftFileName;
            QChar leftFollow = patch.count() > leftStart.count() ? patch.at(leftStart.count()) : QLatin1Char('\n');

            // followed by leftFileRegExp
            if (patch.startsWith(leftStart) && fileNameEnd(leftFollow)) {
                patch.remove(0, patch.indexOf(QLatin1Char('\n'), leftStart.count()) + 1);

                // followed by rightFileRegExp
                const QString rightStart = QString::fromLatin1("+++ b/") + rightFileName;
                QChar rightFollow = patch.count() > rightStart.count() ? patch.at(rightStart.count()) : QLatin1Char('\n');

                // followed by rightFileRegExp
                if (patch.startsWith(rightStart) && fileNameEnd(rightFollow)) {
                    patch.remove(0, patch.indexOf(QLatin1Char('\n'), rightStart.count()) + 1);

                    fileData.chunks = readChunks(patch,
                                                 &fileData.lastChunkAtTheEndOfFile,
                                                 &readOk);
                }
            }
        } else if (copyRenameChunks.isEmpty()) {
            readOk = true;
        }
    }

    if (ok)
        *ok = readOk;

    if (!readOk)
        return FileData();

    return fileData;
}

static QList<FileData> readGitPatch(const QString &patch, bool *ok)
{
    const QRegExp simpleGitRegExp(QLatin1String("(?:\\n|^)diff --git a/([^\\n]+) b/\\1\\n")); // diff --git a/cap1 b/cap1

    const QRegExp similarityRegExp(QLatin1String(
                  "(?:\\n|^)diff --git a/([^\\n]+) b/([^\\n]+)\\n" // diff --git a/cap1 b/cap2
                  "(?:dis)?similarity index \\d{1,3}%\\n"          // similarity / dissimilarity index xxx% (100% max)
                  "(copy|rename) from \\1\\n"                      // copy / rename from cap1
                  "\\3 to \\2\\n"));                               // copy / rename (cap3) to cap2

    bool readOk = false;

    QList<FileData> fileDataList;

    bool simpleGitMatched;
    int pos = 0;
    auto calculateGitMatchAndPosition = [&]() {
        const int simpleGitPos = simpleGitRegExp.indexIn(patch, pos, QRegExp::CaretAtOffset);
        const int similarityPos = similarityRegExp.indexIn(patch, pos, QRegExp::CaretAtOffset);
        if (simpleGitPos < 0) {
            pos = similarityPos;
            simpleGitMatched = false;
            return;
        } else if (similarityPos < 0) {
            pos = simpleGitPos;
            simpleGitMatched = true;
            return;
        }
        pos = qMin(simpleGitPos, similarityPos);
        simpleGitMatched = (pos == simpleGitPos);
    };

    // Set both pos and simpleGitMatched according to the first match:
    calculateGitMatchAndPosition();

    if (pos >= 0) { // git style patch
        readOk = true;
        int endOfLastHeader = 0;
        QString lastLeftFileName;
        QString lastRightFileName;
        FileData::FileOperation lastOperation = FileData::ChangeFile;
        do {
            if (endOfLastHeader > 0) {
                const QString headerAndChunks = patch.mid(endOfLastHeader,
                                                          pos - endOfLastHeader);

                FileData fileData;
                if (lastOperation == FileData::ChangeFile) {
                    fileData = readGitHeaderAndChunks(headerAndChunks,
                                                      lastLeftFileName,
                                                      &readOk);
                } else {
                    fileData = readCopyRenameChunks(headerAndChunks,
                                                    lastOperation,
                                                    lastLeftFileName,
                                                    lastRightFileName,
                                                    &readOk);
                }
                if (!readOk)
                    break;

                fileDataList.append(fileData);
            }

            if (simpleGitMatched) {
                const QString fileName = simpleGitRegExp.cap(1);
                pos += simpleGitRegExp.matchedLength();
                endOfLastHeader = pos;
                lastLeftFileName = fileName;
                lastRightFileName = fileName;
                lastOperation = FileData::ChangeFile;
            } else {
                lastLeftFileName = similarityRegExp.cap(1);
                lastRightFileName = similarityRegExp.cap(2);
                const QString operation = similarityRegExp.cap(3);
                pos += similarityRegExp.matchedLength();
                endOfLastHeader = pos;
                if (operation == QLatin1String("copy"))
                    lastOperation = FileData::CopyFile;
                else if (operation == QLatin1String("rename"))
                    lastOperation = FileData::RenameFile;
                else
                    break; // either copy or rename, otherwise broken
            }

            // give both pos and simpleGitMatched a new value for the next match
            calculateGitMatchAndPosition();
        } while (pos != -1);

        if (endOfLastHeader > 0 && readOk) {
            const QString headerAndChunks = patch.mid(endOfLastHeader,
                                                      patch.count() - endOfLastHeader - 1);

            FileData fileData;
            if (lastOperation == FileData::ChangeFile) {

                fileData = readGitHeaderAndChunks(headerAndChunks,
                                                  lastLeftFileName,
                                                  &readOk);
            } else {
                fileData = readCopyRenameChunks(headerAndChunks,
                                                lastOperation,
                                                lastLeftFileName,
                                                lastRightFileName,
                                                &readOk);
            }
            if (readOk)
                fileDataList.append(fileData);
        }
    }

    if (ok)
        *ok = readOk;

    if (!readOk)
        return QList<FileData>();

    return fileDataList;
}

QList<FileData> DiffUtils::readPatch(const QString &patch, bool *ok)
{
    bool readOk = false;

    QList<FileData> fileDataList;

    QString croppedPatch = patch;
    // Crop e.g. "-- \n2.10.2.windows.1\n\n" at end of file
    const QRegExp formatPatchEndingRegExp(QLatin1String("(\\n-- \\n\\S*\\n\\n$)"));
    const int pos = formatPatchEndingRegExp.indexIn(patch);
    if (pos != -1)
        croppedPatch = patch.left(pos + 1); // crop the ending for git format-patch

    fileDataList = readGitPatch(croppedPatch, &readOk);
    if (!readOk)
        fileDataList = readDiffPatch(croppedPatch, &readOk);

    if (ok)
        *ok = readOk;

    return fileDataList;
}

} // namespace DiffEditor
