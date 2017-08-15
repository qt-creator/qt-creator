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

#include "texteditor/fontsettings.h"
#include "utils/asconst.h"

#include <QFutureInterfaceBase>
#include <QRegularExpression>
#include <QStringList>
#include <QTextStream>

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

static QString leftFileName(const FileData &fileData, unsigned formatFlags)
{
    QString diffText;
    QTextStream str(&diffText);
    if (fileData.fileOperation == FileData::NewFile) {
        str << "/dev/null";
    } else {
        if (formatFlags & DiffUtils::AddLevel)
            str << "a/";
        str << fileData.leftFileInfo.fileName;
    }
    return diffText;
}

static QString rightFileName(const FileData &fileData, unsigned formatFlags)
{
    QString diffText;
    QTextStream str(&diffText);
    if (fileData.fileOperation == FileData::DeleteFile) {
        str << "/dev/null";
    } else {
        if (formatFlags & DiffUtils::AddLevel)
            str << "b/";
        str << fileData.rightFileInfo.fileName;
    }
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
        if (fileData.fileOperation == FileData::NewFile
                || fileData.fileOperation == FileData::DeleteFile) { // git only?
            if (fileData.fileOperation == FileData::NewFile)
                str << "new";
            else
                str << "deleted";
            str << " file mode 100644\n";
        }
        str << "index " << fileData.leftFileInfo.typeInfo << ".." << fileData.rightFileInfo.typeInfo;
        if (fileData.fileOperation == FileData::ChangeFile)
            str << " 100644";
        str << "\n";

        if (fileData.binaryFiles) {
            str << "Binary files ";
            str << leftFileName(fileData, formatFlags);
            str << " and ";
            str << rightFileName(fileData, formatFlags);
            str << " differ\n";
        } else {
            if (!fileData.chunks.isEmpty()) {
                str << "--- ";
                str << leftFileName(fileData, formatFlags) << "\n";
                str << "+++ ";
                str << rightFileName(fileData, formatFlags) << "\n";
                for (int j = 0; j < fileData.chunks.count(); j++) {
                    str << makePatch(fileData.chunks.at(j),
                                     (j == fileData.chunks.count() - 1)
                                     && fileData.lastChunkAtTheEndOfFile);
                }
            }
        }
    }
    return diffText;
}

static QList<RowData> readLines(QStringRef patch,
                                bool lastChunk,
                                bool *lastChunkAtTheEndOfFile,
                                bool *ok)
{
    QList<Diff> diffList;

    const QChar newLine = QLatin1Char('\n');

    int lastEqual = -1;
    int lastDelete = -1;
    int lastInsert = -1;

    int noNewLineInEqual = -1;
    int noNewLineInDelete = -1;
    int noNewLineInInsert = -1;

    const QVector<QStringRef> lines = patch.split(newLine);
    int i;
    for (i = 0; i < lines.count(); i++) {
        QStringRef line = lines.at(i);
        if (line.isEmpty()) { // need to have at least one character (1 column)
            if (lastChunk)
                i = lines.count(); // pretend as we've read all the lines (we just ignore the rest)
            break;
        }
        const QChar firstCharacter = line.at(0);
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

            Diff diffToBeAdded(command, line.mid(1).toString() + newLine);

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

static QStringRef readLine(QStringRef text, QStringRef *remainingText, bool *hasNewLine)
{
    const QChar newLine('\n');
    const int indexOfFirstNewLine = text.indexOf(newLine);
    if (indexOfFirstNewLine < 0) {
        if (remainingText)
            *remainingText = QStringRef();
        if (hasNewLine)
            *hasNewLine = false;
        return text;
    }

    if (hasNewLine)
        *hasNewLine = true;

    if (remainingText)
        *remainingText = text.mid(indexOfFirstNewLine + 1);

    return text.left(indexOfFirstNewLine);
}

static bool detectChunkData(QStringRef chunkDiff,
                            ChunkData *chunkData,
                            QStringRef *remainingPatch)
{
    bool hasNewLine;
    const QStringRef chunkLine = readLine(chunkDiff, remainingPatch, &hasNewLine);

    const QLatin1String leftPosMarker("@@ -");
    const QLatin1String rightPosMarker(" +");
    const QLatin1String optionalHintMarker(" @@");

    const int leftPosIndex = chunkLine.indexOf(leftPosMarker);
    if (leftPosIndex != 0)
        return false;

    const int rightPosIndex = chunkLine.indexOf(rightPosMarker, leftPosIndex + leftPosMarker.size());
    if (rightPosIndex < 0)
        return false;

    const int optionalHintIndex = chunkLine.indexOf(optionalHintMarker, rightPosIndex + rightPosMarker.size());
    if (optionalHintIndex < 0)
        return false;

    const int leftPosStart = leftPosIndex + leftPosMarker.size();
    const int leftPosLength = rightPosIndex - leftPosStart;
    QStringRef leftPos = chunkLine.mid(leftPosStart, leftPosLength);

    const int rightPosStart = rightPosIndex + rightPosMarker.size();
    const int rightPosLength = optionalHintIndex - rightPosStart;
    QStringRef rightPos = chunkLine.mid(rightPosStart, rightPosLength);

    const int optionalHintStart = optionalHintIndex + optionalHintMarker.size();
    const int optionalHintLength = chunkLine.size() - optionalHintStart;
    const QStringRef optionalHint = chunkLine.mid(optionalHintStart, optionalHintLength);

    const QChar comma(',');
    bool ok;

    const int leftCommaIndex = leftPos.indexOf(comma);
    if (leftCommaIndex >= 0)
        leftPos = leftPos.left(leftCommaIndex);
    const int leftLineNumber = leftPos.toString().toInt(&ok);
    if (!ok)
        return false;

    const int rightCommaIndex = rightPos.indexOf(comma);
    if (rightCommaIndex >= 0)
        rightPos = rightPos.left(rightCommaIndex);
    const int rightLineNumber = rightPos.toString().toInt(&ok);
    if (!ok)
        return false;

    chunkData->leftStartingLineNumber = leftLineNumber - 1;
    chunkData->rightStartingLineNumber = rightLineNumber - 1;
    chunkData->contextInfo = optionalHint.toString();

    return true;
}

static QList<ChunkData> readChunks(QStringRef patch,
                                   bool *lastChunkAtTheEndOfFile,
                                   bool *ok)
{
    QList<ChunkData> chunkDataList;
    int position = -1;

    QVector<int> startingPositions; // store starting positions of @@
    if (patch.startsWith(QStringLiteral("@@ -")))
        startingPositions.append(position + 1);

    while ((position = patch.indexOf(QStringLiteral("\n@@ -"), position + 1)) >= 0)
        startingPositions.append(position + 1);

    const QChar newLine('\n');
    bool readOk = true;

    const int count = startingPositions.count();
    for (int i = 0; i < count; i++) {
        const int chunkStart = startingPositions.at(i);
        const int chunkEnd = (i < count - 1)
                // drop the newline before the next chunk start
                ? startingPositions.at(i + 1) - 1
                // drop the possible newline by the end of patch
                : (patch.at(patch.count() - 1) == newLine ? patch.count() - 1 : patch.count());

        // extract just one chunk
        const QStringRef chunkDiff = patch.mid(chunkStart, chunkEnd - chunkStart);

        ChunkData chunkData;
        QStringRef lines;
        readOk = detectChunkData(chunkDiff, &chunkData, &lines);

        if (!readOk)
            break;

        chunkData.rows = readLines(lines, i == (startingPositions.size() - 1),
                                   lastChunkAtTheEndOfFile, &readOk);
        if (!readOk)
            break;

        chunkDataList.append(chunkData);
    }

    if (ok)
        *ok = readOk;

    return chunkDataList;
}

static FileData readDiffHeaderAndChunks(QStringRef headerAndChunks,
                                        bool *ok)
{
    QStringRef patch = headerAndChunks;
    FileData fileData;
    bool readOk = false;

    const QRegularExpression leftFileRegExp(
          "(?:\\n|^)-{3} "       // "--- "
          "([^\\t\\n]+)"         // "fileName1"
          "(?:\\t[^\\n]*)*\\n"); // optionally followed by: \t anything \t anything ...)
    const QRegularExpression rightFileRegExp(
          "^\\+{3} "             // "+++ "
          "([^\\t\\n]+)"         // "fileName2"
          "(?:\\t[^\\n]*)*\\n"); // optionally followed by: \t anything \t anything ...)
    const QRegularExpression binaryRegExp(
          "^Binary files ([^\\t\\n]+) and ([^\\t\\n]+) differ$");

    // followed either by leftFileRegExp
    const QRegularExpressionMatch leftMatch = leftFileRegExp.match(patch);
    if (leftMatch.hasMatch() && leftMatch.capturedStart() == 0) {
        patch = patch.mid(leftMatch.capturedEnd());
        fileData.leftFileInfo.fileName = leftMatch.captured(1);

        // followed by rightFileRegExp
        const QRegularExpressionMatch rightMatch = rightFileRegExp.match(patch);
        if (rightMatch.hasMatch() && rightMatch.capturedStart() == 0) {
            patch = patch.mid(rightMatch.capturedEnd());
            fileData.rightFileInfo.fileName = rightMatch.captured(1);

            fileData.chunks = readChunks(patch,
                                         &fileData.lastChunkAtTheEndOfFile,
                                         &readOk);
        }
    } else {
        // or by binaryRegExp
        const QRegularExpressionMatch binaryMatch = binaryRegExp.match(patch);
        if (binaryMatch.hasMatch() && binaryMatch.capturedStart() == 0) {
            fileData.leftFileInfo.fileName = binaryMatch.captured(1);
            fileData.rightFileInfo.fileName = binaryMatch.captured(2);
            fileData.binaryFiles = true;
            readOk = true;
        }
    }

    if (ok)
        *ok = readOk;

    if (!readOk)
        return FileData();

    return fileData;

}

static QList<FileData> readDiffPatch(QStringRef patch,
                                     bool *ok,
                                     QFutureInterfaceBase *jobController)
{
    const QRegularExpression diffRegExp("(?:\\n|^)"          // new line of the beginning of a patch
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
                                        ")");                // end of or

    bool readOk = false;

    QList<FileData> fileDataList;

    QRegularExpressionMatch diffMatch = diffRegExp.match(patch);
    if (diffMatch.hasMatch()) {
        readOk = true;
        int lastPos = -1;
        do {
            if (jobController && jobController->isCanceled())
                return QList<FileData>();

            int pos = diffMatch.capturedStart();
            if (lastPos >= 0) {
                QStringRef headerAndChunks = patch.mid(lastPos,
                                                       pos - lastPos);

                const FileData fileData = readDiffHeaderAndChunks(headerAndChunks,
                                                                  &readOk);

                if (!readOk)
                    break;

                fileDataList.append(fileData);
            }
            lastPos = pos;
            pos = diffMatch.capturedEnd();
            diffMatch = diffRegExp.match(patch, pos);
        } while (diffMatch.hasMatch());

        if (readOk) {
            QStringRef headerAndChunks = patch.mid(lastPos,
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

// The git diff patch format (ChangeFile, NewFile, DeleteFile)
// 0.  <some text lines to skip, e.g. show description>\n
// 1.  diff --git a/[fileName] b/[fileName]\n
// 2a. new file mode [fileModeNumber]\n
// 2b. deleted file mode [fileModeNumber]\n
// 2c. old mode [oldFileModeNumber]\n
//     new mode [newFileModeNumber]\n
// 2d. <Nothing, only in case of ChangeFile>
// 3a.  index [leftIndexSha]..[rightIndexSha] <optionally: octalNumber>
// 3b. <Nothing, only in case of ChangeFile, "Dirty submodule" case>
// 4a. <Nothing more, only possible in case of NewFile or DeleteFile> ???
// 4b. \nBinary files [leftFileNameOrDevNull] and [rightFileNameOrDevNull] differ
// 4c. --- [leftFileNameOrDevNull]\n
//     +++ [rightFileNameOrDevNull]\n
//     <Chunks>

// The git diff patch format (CopyFile, RenameFile)
// 0.  [some text lines to skip, e.g. show description]\n
// 1.  diff --git a/[leftFileName] b/[rightFileName]\n
// 2.  [dis]similarity index [0-100]%\n
//     [copy / rename] from [leftFileName]\n
//     [copy / rename] to [rightFileName]
// 3a. <Nothing more, only when similarity index was 100%>
// 3b. index [leftIndexSha]..[rightIndexSha] <optionally: octalNumber>
// 4.  --- [leftFileNameOrDevNull]\n
//     +++ [rightFileNameOrDevNull]\n
//     <Chunks>

static bool detectIndexAndBinary(QStringRef patch,
                                 FileData *fileData,
                                 QStringRef *remainingPatch)
{
    bool hasNewLine;
    *remainingPatch = patch;

    if (remainingPatch->isEmpty() && (fileData->fileOperation == FileData::CopyFile
                            || fileData->fileOperation == FileData::RenameFile)) {
        // in case of 100% similarity we don't have more lines in the patch
        return true;
    }

    QStringRef afterNextLine;
    // index [leftIndexSha]..[rightIndexSha] <optionally: octalNumber>
    const QStringRef nextLine = readLine(patch, &afterNextLine, &hasNewLine);

    const QLatin1String indexHeader("index ");

    if (nextLine.startsWith(indexHeader)) {
        const QStringRef indices = nextLine.mid(indexHeader.size());
        const int dotsPosition = indices.indexOf(QStringLiteral(".."));
        if (dotsPosition < 0)
            return false;
        fileData->leftFileInfo.typeInfo = indices.left(dotsPosition).toString();

        // if there is no space we take the remaining string
        const int spacePosition = indices.indexOf(QChar::Space, dotsPosition + 2);
        const int length = spacePosition < 0 ? -1 : spacePosition - dotsPosition - 2;
        fileData->rightFileInfo.typeInfo = indices.mid(dotsPosition + 2, length).toString();

        *remainingPatch = afterNextLine;
    } else if (fileData->fileOperation != FileData::ChangeFile) {
        // no index only in case of ChangeFile,
        // the dirty submodule diff case, see "Dirty Submodule" test:
        return false;
    }

    if (remainingPatch->isEmpty() && (fileData->fileOperation == FileData::NewFile
                            || fileData->fileOperation == FileData::DeleteFile)) {
        // OK in case of empty file
        return true;
    }

    const QString devNull("/dev/null");
    const QString leftFileName = fileData->fileOperation == FileData::NewFile
            ? devNull : QLatin1String("a/") + fileData->leftFileInfo.fileName;
    const QString rightFileName = fileData->fileOperation == FileData::DeleteFile
            ? devNull : QLatin1String("b/") + fileData->rightFileInfo.fileName;

    const QString binaryLine = QLatin1String("Binary files ")
            + leftFileName + QLatin1String(" and ")
            + rightFileName + QLatin1String(" differ");

    if (*remainingPatch == binaryLine) {
        fileData->binaryFiles = true;
        *remainingPatch = QStringRef();
        return true;
    }

    const QString leftStart = QLatin1String("--- ") + leftFileName;
    QStringRef afterMinuses;
    // --- leftFileName
    const QStringRef minuses = readLine(*remainingPatch, &afterMinuses, &hasNewLine);
    if (!hasNewLine)
        return false; // we need to have at least one more line

    if (!minuses.startsWith(leftStart))
        return false;

    const QString rightStart = QLatin1String("+++ ") + rightFileName;
    QStringRef afterPluses;
    // +++ rightFileName
    const QStringRef pluses = readLine(afterMinuses, &afterPluses, &hasNewLine);
    if (!hasNewLine)
        return false; // we need to have at least one more line

    if (!pluses.startsWith(rightStart))
        return false;

    *remainingPatch = afterPluses;
    return true;
}

static bool extractCommonFileName(QStringRef fileNames, QStringRef *fileName)
{
    // we should have 1 space between filenames
    if (fileNames.size() % 2 == 0)
        return false;

    if (!fileNames.startsWith(QStringLiteral("a/")))
        return false;

    // drop the space in between
    const int fileNameSize = fileNames.size() / 2;
    if (!fileNames.mid(fileNameSize).startsWith(" b/"))
        return false;

    // drop "a/"
    const QStringRef leftFileName = fileNames.mid(2, fileNameSize - 2);

    // drop the first filename + " b/"
    const QStringRef rightFileName = fileNames.mid(fileNameSize + 3, fileNameSize - 2);

    if (leftFileName != rightFileName)
        return false;

    *fileName = leftFileName;
    return true;
}

static bool detectFileData(QStringRef patch,
                           FileData *fileData,
                           QStringRef *remainingPatch) {
    bool hasNewLine;

    QStringRef afterDiffGit;
    // diff --git a/leftFileName b/rightFileName
    const QStringRef diffGit = readLine(patch, &afterDiffGit, &hasNewLine);
    if (!hasNewLine)
        return false; // we need to have at least one more line

    const QLatin1String gitHeader("diff --git ");
    const QStringRef fileNames = diffGit.mid(gitHeader.size());
    QStringRef commonFileName;
    if (extractCommonFileName(fileNames, &commonFileName)) {
        // change / new / delete

        fileData->fileOperation = FileData::ChangeFile;
        fileData->leftFileInfo.fileName = fileData->rightFileInfo.fileName = commonFileName.toString();

        QStringRef afterSecondLine;
        const QStringRef secondLine = readLine(afterDiffGit, &afterSecondLine, &hasNewLine);
        if (!hasNewLine)
            return false; // we need to have at least one more line

        if (secondLine.startsWith(QStringLiteral("new file mode "))) {
            fileData->fileOperation = FileData::NewFile;
            *remainingPatch = afterSecondLine;
        } else if (secondLine.startsWith(QStringLiteral("deleted file mode "))) {
            fileData->fileOperation = FileData::DeleteFile;
            *remainingPatch = afterSecondLine;
        } else if (secondLine.startsWith(QStringLiteral("old mode "))) {
            QStringRef afterThirdLine;
            // new mode
            readLine(afterSecondLine, &afterThirdLine, &hasNewLine);
            if (!hasNewLine)
                return false; // we need to have at least one more line

            // TODO: validate new mode line
            *remainingPatch = afterThirdLine;
        } else {
            *remainingPatch = afterDiffGit;
        }

    } else {
        // copy / rename

        QStringRef afterSimilarity;
        // (dis)similarity index [0-100]%
        readLine(afterDiffGit, &afterSimilarity, &hasNewLine);
        if (!hasNewLine)
            return false; // we need to have at least one more line

        // TODO: validate similarity line

        QStringRef afterCopyRenameFrom;
        // [copy / rename] from leftFileName
        const QStringRef copyRenameFrom = readLine(afterSimilarity, &afterCopyRenameFrom, &hasNewLine);
        if (!hasNewLine)
            return false; // we need to have at least one more line

        const QLatin1String copyFrom("copy from ");
        const QLatin1String renameFrom("rename from ");
        if (copyRenameFrom.startsWith(copyFrom)) {
            fileData->fileOperation = FileData::CopyFile;
            fileData->leftFileInfo.fileName = copyRenameFrom.mid(copyFrom.size()).toString();
        } else if (copyRenameFrom.startsWith(renameFrom)) {
            fileData->fileOperation = FileData::RenameFile;
            fileData->leftFileInfo.fileName = copyRenameFrom.mid(renameFrom.size()).toString();
        } else {
            return false;
        }

        QStringRef afterCopyRenameTo;
        // [copy / rename] to rightFileName
        const QStringRef copyRenameTo = readLine(afterCopyRenameFrom, &afterCopyRenameTo, &hasNewLine);

        // if (dis)similarity index is 100% we don't have more lines

        const QLatin1String copyTo("copy to ");
        const QLatin1String renameTo("rename to ");
        if (fileData->fileOperation == FileData::CopyFile && copyRenameTo.startsWith(copyTo)) {
            fileData->rightFileInfo.fileName = copyRenameTo.mid(copyTo.size()).toString();
        } else if (fileData->fileOperation == FileData::RenameFile && copyRenameTo.startsWith(renameTo)) {
            fileData->rightFileInfo.fileName = copyRenameTo.mid(renameTo.size()).toString();
        } else {
            return false;
        }

        *remainingPatch = afterCopyRenameTo;
    }
    return detectIndexAndBinary(*remainingPatch, fileData, remainingPatch);
}

static QList<FileData> readGitPatch(QStringRef patch, bool *ok,
                                    QFutureInterfaceBase *jobController)
{
    int position = -1;

    QVector<int> startingPositions; // store starting positions of git headers
    if (patch.startsWith(QStringLiteral("diff --git ")))
        startingPositions.append(position + 1);

    while ((position = patch.indexOf(QStringLiteral("\ndiff --git "), position + 1)) >= 0)
        startingPositions.append(position + 1);

    class PatchInfo {
    public:
        QStringRef patch;
        FileData fileData;
    };

    const QChar newLine('\n');
    bool readOk = true;

    QVector<PatchInfo> patches;
    const int count = startingPositions.count();
    for (int i = 0; i < count; i++) {
        if (jobController && jobController->isCanceled())
            return QList<FileData>();

        const int diffStart = startingPositions.at(i);
        const int diffEnd = (i < count - 1)
                // drop the newline before the next header start
                ? startingPositions.at(i + 1) - 1
                // drop the possible newline by the end of file
                : (patch.at(patch.count() - 1) == newLine ? patch.count() - 1 : patch.count());

        // extract the patch for just one file
        const QStringRef fileDiff = patch.mid(diffStart, diffEnd - diffStart);

        FileData fileData;
        QStringRef remainingFileDiff;
        readOk = detectFileData(fileDiff, &fileData, &remainingFileDiff);

        if (!readOk)
            break;

        patches.append(PatchInfo { remainingFileDiff, fileData });
    }

    if (!readOk) {
        if (ok)
            *ok = readOk;
        return QList<FileData>();
    }

    if (jobController)
        jobController->setProgressRange(0, patches.count());

    QList<FileData> fileDataList;
    readOk = false;
    int i = 0;
    for (const auto &patchInfo : Utils::asConst(patches)) {
        if (jobController) {
            if (jobController->isCanceled())
                return QList<FileData>();
            jobController->setProgressValue(i++);
        }

        FileData fileData = patchInfo.fileData;
        if (!patchInfo.patch.isEmpty() || fileData.fileOperation == FileData::ChangeFile)
            fileData.chunks = readChunks(patchInfo.patch, &fileData.lastChunkAtTheEndOfFile, &readOk);
        else
            readOk = true;

        if (!readOk)
            break;

        fileDataList.append(fileData);
    }

    if (ok)
        *ok = readOk;

    if (!readOk)
        return QList<FileData>();

    return fileDataList;
}

QList<FileData> DiffUtils::readPatch(const QString &patch, bool *ok,
                                     QFutureInterfaceBase *jobController)
{
    bool readOk = false;

    QList<FileData> fileDataList;

    if (jobController) {
        jobController->setProgressRange(0, 1);
        jobController->setProgressValue(0);
    }
    QStringRef croppedPatch(&patch);
    // Crop e.g. "-- \n2.10.2.windows.1\n\n" at end of file
    const QRegularExpression formatPatchEndingRegExp("(\\n-- \\n\\S*\\n\\n$)");
    const QRegularExpressionMatch match = formatPatchEndingRegExp.match(croppedPatch);
    if (match.hasMatch())
        croppedPatch = croppedPatch.left(match.capturedStart() + 1);

    fileDataList = readGitPatch(croppedPatch, &readOk, jobController);
    if (!readOk)
        fileDataList = readDiffPatch(croppedPatch, &readOk, jobController);

    if (ok)
        *ok = readOk;

    return fileDataList;
}

} // namespace DiffEditor
