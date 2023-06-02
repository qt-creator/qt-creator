// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "diffutils.h"

#include <utils/algorithm.h>
#include <utils/differ.h>

#include <QFuture>
#include <QPromise>
#include <QRegularExpression>
#include <QStringList>
#include <QTextStream>

using namespace Utils;

namespace DiffEditor {

static int forBlockNumber(const QMap<int, QPair<int, int>> &chunkInfo, int blockNumber,
                          const std::function<int (int, int, int)> &func)
{
    if (chunkInfo.isEmpty())
        return -1;

    auto it = chunkInfo.upperBound(blockNumber);
    if (it == chunkInfo.constBegin())
        return -1;

    --it;

    if (blockNumber < it.key() + it.value().first)
        return func(it.key(), it.value().first, it.value().second);

    return -1;
}

int DiffChunkInfo::chunkRowForBlockNumber(int blockNumber) const
{
    return forBlockNumber(m_chunkInfo, blockNumber, [blockNumber](int startBlockNumber, int, int)
                          { return blockNumber - startBlockNumber; });
}

int DiffChunkInfo::chunkRowsCountForBlockNumber(int blockNumber) const
{
    return forBlockNumber(m_chunkInfo, blockNumber,
                          [](int, int rowsCount, int) { return rowsCount; });
}

int DiffChunkInfo::chunkIndexForBlockNumber(int blockNumber) const
{
    return forBlockNumber(m_chunkInfo, blockNumber,
                          [](int, int, int chunkIndex) { return chunkIndex; });
}

int ChunkSelection::selectedRowsCount() const
{
    return Utils::toSet(selection[LeftSide]).unite(Utils::toSet(selection[RightSide])).size();
}

static QList<TextLineData> assemblyRows(const QList<TextLineData> &lines,
                                        const QMap<int, int> &lineSpans)
{
    QList<TextLineData> data;

    const int lineCount = lines.size();
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
    const bool leftLineEqual = leftLines.isEmpty() || leftLines.last().text.isEmpty();
    const bool rightLineEqual = rightLines.isEmpty() || rightLines.last().text.isEmpty();
    return leftLineEqual && rightLineEqual;
}

static void handleLine(const QStringList &newLines, int line, QList<TextLineData> *lines,
                       int *lineNumber)
{
    if (line < newLines.size()) {
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

static void handleDifference(const QString &text, QList<TextLineData> *lines, int *lineNumber)
{
    const QStringList newLines = text.split('\n');
    for (int line = 0; line < newLines.size(); ++line) {
        const int startPos = line > 0 ? -1 : lines->isEmpty() ? 0 : lines->last().text.size();
        handleLine(newLines, line, lines, lineNumber);
        const int endPos = line < newLines.size() - 1
                               ? -1
                               : lines->isEmpty() ? 0 : lines->last().text.size();
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

    while (i <= leftDiffList.size() && j <= rightDiffList.size()) {
        const Diff leftDiff = i < leftDiffList.size() ? leftDiffList.at(i) : Diff(Diff::Equal);
        const Diff rightDiff = j < rightDiffList.size() ? rightDiffList.at(j) : Diff(Diff::Equal);

        if (leftDiff.command == Diff::Delete) {
            if (j == rightDiffList.size() && lastLineEqual && leftDiff.text.startsWith('\n'))
                equalLines.insert(leftLineNumber, rightLineNumber);
            // process delete
            handleDifference(leftDiff.text, &leftLines, &leftLineNumber);
            lastLineEqual = lastLinesEqual(leftLines, rightLines);
            if (j == rightDiffList.size())
                lastLineEqual = false;
            i++;
        }
        if (rightDiff.command == Diff::Insert) {
            if (i == leftDiffList.size() && lastLineEqual && rightDiff.text.startsWith('\n'))
                equalLines.insert(leftLineNumber, rightLineNumber);
            // process insert
            handleDifference(rightDiff.text, &rightLines, &rightLineNumber);
            lastLineEqual = lastLinesEqual(leftLines, rightLines);
            if (i == leftDiffList.size())
                lastLineEqual = false;
            j++;
        }
        if (leftDiff.command == Diff::Equal && rightDiff.command == Diff::Equal) {
            // process equal
            const QStringList newLeftLines = leftDiff.text.split('\n');
            const QStringList newRightLines = rightDiff.text.split('\n');

            int line = 0;

            if (i < leftDiffList.size() || j < rightDiffList.size()
                || (!leftLines.isEmpty() && !rightLines.isEmpty())) {
                while (line < qMax(newLeftLines.size(), newRightLines.size())) {
                    handleLine(newLeftLines, line, &leftLines, &leftLineNumber);
                    handleLine(newRightLines, line, &rightLines, &rightLineNumber);

                    const int commonLineCount = qMin(newLeftLines.size(), newRightLines.size());
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
                                if (i == leftDiffList.size() && j == rightDiffList.size())
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
                    if ((line < commonLineCount - 1)    // before the last common line in equality
                        || (line == commonLineCount - 1 // or the last common line in equality
                            && i == leftDiffList.size() // and it's the last iteration
                            && j == rightDiffList.size())) {
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

    QList<TextLineData> leftData = assemblyRows(leftLines, leftSpans);
    QList<TextLineData> rightData = assemblyRows(rightLines, rightSpans);

    // fill ending separators
    for (int i = leftData.size(); i < rightData.size(); i++)
        leftData.append(TextLineData(TextLineData::Separator));
    for (int i = rightData.size(); i < leftData.size(); i++)
        rightData.append(TextLineData(TextLineData::Separator));

    const int visualLineCount = leftData.size();
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
    while (i < originalData.rows.size()) {
        const RowData &row = originalData.rows[i];
        if (row.equal) {
            // count how many equal
            int equalRowStart = i;
            i++;
            while (i < originalData.rows.size()) {
                const RowData originalRow = originalData.rows.at(i);
                if (!originalRow.equal)
                    break;
                i++;
            }
            const bool first = equalRowStart == 0; // includes first line?
            const bool last = i == originalData.rows.size(); // includes last line?

            const int firstLine = first
                    ? 0 : equalRowStart + contextLineCount;
            const int lastLine = last ? originalData.rows.size() : i - contextLineCount;

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
    while (i < originalData.rows.size()) {
        const bool contextChunk = hiddenRows.contains(i);
        ChunkData chunkData;
        chunkData.contextChunk = contextChunk;
        chunkData.startingLineNumber = {leftLineNumber, rightLineNumber};
        while (i < originalData.rows.size()) {
            if (contextChunk != hiddenRows.contains(i))
                break;
            RowData rowData = originalData.rows.at(i);
            chunkData.rows.append(rowData);
            if (rowData.line[LeftSide].textLineType == TextLineData::TextLine)
                ++leftLineNumber;
            if (rowData.line[RightSide].textLineType == TextLineData::TextLine)
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
        line = startLineCharacter + textLine + '\n';
        if (addNoNewline)
            line += "\\ No newline at end of file\n";
    }

    return line;
}

QString DiffUtils::makePatch(const ChunkData &chunkData,
                             bool lastChunk)
{
    if (chunkData.contextChunk)
        return {};

    QString diffText;
    int leftLineCount = 0;
    int rightLineCount = 0;
    QList<TextLineData> leftBuffer, rightBuffer;

    int rowToBeSplit = -1;

    if (lastChunk) {
        // Detect the case when the last equal line is followed by
        // only separators on left or on right. In that case
        // the last equal line needs to be split.
        const int rowCount = chunkData.rows.size();
        int i = 0;
        for (i = rowCount; i > 0; i--) {
            const RowData &rowData = chunkData.rows.at(i - 1);
            if (rowData.line[LeftSide].textLineType != TextLineData::Separator
                    || rowData.line[RightSide].textLineType != TextLineData::TextLine)
                break;
        }
        const int leftSeparator = i;
        for (i = rowCount; i > 0; i--) {
            const RowData &rowData = chunkData.rows.at(i - 1);
            if (rowData.line[RightSide].textLineType != TextLineData::Separator
                    || rowData.line[LeftSide].textLineType != TextLineData::TextLine)
                break;
        }
        const int rightSeparator = i;
        const int commonSeparator = qMin(leftSeparator, rightSeparator);
        if (commonSeparator > 0
                && commonSeparator < rowCount
                && chunkData.rows.at(commonSeparator - 1).equal)
            rowToBeSplit = commonSeparator - 1;
    }

    for (int i = 0; i <= chunkData.rows.size(); i++) {
        const RowData &rowData = i < chunkData.rows.size()
                                     ? chunkData.rows.at(i)
                                     : RowData(TextLineData(TextLineData::Separator)); // dummy,
        // ensure we process buffers to the end.
        // rowData will be equal
        if (rowData.equal && i != rowToBeSplit) {
            if (!leftBuffer.isEmpty()) {
                for (int j = 0; j < leftBuffer.size(); j++) {
                    const QString line = makePatchLine('-',
                                                       leftBuffer.at(j).text,
                                                       lastChunk,
                                                       i == chunkData.rows.size()
                                                           && j == leftBuffer.size() - 1);

                    if (!line.isEmpty())
                        ++leftLineCount;

                    diffText += line;
                }
                leftBuffer.clear();
            }
            if (!rightBuffer.isEmpty()) {
                for (int j = 0; j < rightBuffer.size(); j++) {
                    const QString line = makePatchLine('+',
                                                       rightBuffer.at(j).text,
                                                       lastChunk,
                                                       i == chunkData.rows.size()
                                                           && j == rightBuffer.size() - 1);

                    if (!line.isEmpty())
                        ++rightLineCount;

                    diffText += line;
                }
                rightBuffer.clear();
            }
            if (i < chunkData.rows.size()) {
                const QString line = makePatchLine(' ',
                                                   rowData.line[RightSide].text,
                                                   lastChunk,
                                                   i == chunkData.rows.size() - 1);

                if (!line.isEmpty()) {
                    ++leftLineCount;
                    ++rightLineCount;
                }

                diffText += line;
            }
        } else {
            if (rowData.line[LeftSide].textLineType == TextLineData::TextLine)
                leftBuffer.append(rowData.line[LeftSide]);
            if (rowData.line[RightSide].textLineType == TextLineData::TextLine)
                rightBuffer.append(rowData.line[RightSide]);
        }
    }

    const QString chunkLine = "@@ -"
            + QString::number(chunkData.startingLineNumber[LeftSide] + 1)
            + ','
            + QString::number(leftLineCount)
            + " +"
            + QString::number(chunkData.startingLineNumber[RightSide] + 1)
            + ','
            + QString::number(rightLineCount)
            + " @@"
            + chunkData.contextInfo
            + '\n';

    diffText.prepend(chunkLine);

    return diffText;
}

QString DiffUtils::makePatch(const ChunkData &chunkData,
                             const QString &leftFileName,
                             const QString &rightFileName,
                             bool lastChunk)
{
    QString diffText = makePatch(chunkData, lastChunk);

    const QString rightFileInfo = "+++ " + rightFileName + '\n';
    const QString leftFileInfo = "--- " + leftFileName + '\n';

    diffText.prepend(rightFileInfo);
    diffText.prepend(leftFileInfo);

    return diffText;
}

static QString sideFileName(DiffSide side, const FileData &fileData)
{
    const FileData::FileOperation operation = side == LeftSide ? FileData::NewFile
                                                               : FileData::DeleteFile;
    if (fileData.fileOperation == operation)
        return "/dev/null";
    const QString sideMarker = side == LeftSide ? QString("a/") : QString("b/");
    return sideMarker + fileData.fileInfo[side].fileName;
}

QString DiffUtils::makePatch(const QList<FileData> &fileDataList)
{
    QString diffText;
    QTextStream str(&diffText);

    for (int i = 0; i < fileDataList.size(); i++) {
        const FileData &fileData = fileDataList.at(i);
        str << "diff --git a/" << fileData.fileInfo[LeftSide].fileName
                      << " b/" << fileData.fileInfo[RightSide].fileName << '\n';
        if (fileData.fileOperation == FileData::NewFile
                || fileData.fileOperation == FileData::DeleteFile) { // git only?
            if (fileData.fileOperation == FileData::NewFile)
                str << "new";
            else
                str << "deleted";
            str << " file mode 100644\n";
        }
        str << "index " << fileData.fileInfo[LeftSide].typeInfo << ".." << fileData.fileInfo[RightSide].typeInfo;
        if (fileData.fileOperation == FileData::ChangeFile)
            str << " 100644";
        str << "\n";

        if (fileData.binaryFiles) {
            str << "Binary files ";
            str << sideFileName(LeftSide, fileData);
            str << " and ";
            str << sideFileName(RightSide, fileData);
            str << " differ\n";
        } else {
            if (!fileData.chunks.isEmpty()) {
                str << "--- ";
                str << sideFileName(LeftSide, fileData) << "\n";
                str << "+++ ";
                str << sideFileName(RightSide, fileData) << "\n";
                for (int j = 0; j < fileData.chunks.size(); j++) {
                    str << makePatch(fileData.chunks.at(j),
                                     (j == fileData.chunks.size() - 1)
                                         && fileData.lastChunkAtTheEndOfFile);
                }
            }
        }
    }
    return diffText;
}

static QList<RowData> readLines(QStringView patch, bool lastChunk, bool *lastChunkAtTheEndOfFile, bool *ok)
{
    QList<Diff> diffList;

    const QChar newLine = '\n';

    int lastEqual = -1;
    int lastDelete = -1;
    int lastInsert = -1;

    int noNewLineInEqual = -1;
    int noNewLineInDelete = -1;
    int noNewLineInInsert = -1;

    const QList<QStringView> lines = patch.split(newLine);
    int i;
    for (i = 0; i < lines.size(); i++) {
        QStringView line = lines.at(i);
        if (line.isEmpty()) { // need to have at least one character (1 column)
            if (lastChunk)
                i = lines.size(); // pretend as we've read all the lines (we just ignore the rest)
            break;
        }
        const QChar firstCharacter = line.at(0);
        if (firstCharacter == '\\') { // no new line marker
            if (!lastChunk) // can only appear in last chunk of the file
                break;
            if (!diffList.isEmpty()) {
                Diff &last = diffList.last();
                if (last.text.isEmpty())
                    break;

                if (last.command == Diff::Equal) {
                    if (noNewLineInEqual >= 0)
                        break;
                    noNewLineInEqual = diffList.size() - 1;
                } else if (last.command == Diff::Delete) {
                    if (noNewLineInDelete >= 0)
                        break;
                    noNewLineInDelete = diffList.size() - 1;
                } else if (last.command == Diff::Insert) {
                    if (noNewLineInInsert >= 0)
                        break;
                    noNewLineInInsert = diffList.size() - 1;
                }
            }
        } else {
            Diff::Command command = Diff::Equal;
            if (firstCharacter == ' ') { // common line
                command = Diff::Equal;
            } else if (firstCharacter == '-') { // deleted line
                command = Diff::Delete;
            } else if (firstCharacter == '+') { // inserted line
                command = Diff::Insert;
            } else { // no other character may exist as the first character
                if (lastChunk)
                    i = lines.size(); // pretend as we've read all the lines (we just ignore the rest)
                break;
            }

            Diff diffToBeAdded(command, line.mid(1).toString() + newLine);

            if (!diffList.isEmpty() && diffList.last().command == command)
                diffList.last().text.append(diffToBeAdded.text);
            else
                diffList.append(diffToBeAdded);

            if (command == Diff::Equal) // common line
                lastEqual = diffList.size() - 1;
            else if (command == Diff::Delete) // deleted line
                lastDelete = diffList.size() - 1;
            else if (command == Diff::Insert) // inserted line
                lastInsert = diffList.size() - 1;
        }
    }

    if (i < lines.size() // we broke before
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
        return {};
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
        diff.text = diff.text.left(diff.text.size() - 1);
    }
    if (removeNewLineFromLastDelete) {
        Diff &diff = diffList[lastDelete];
        diff.text = diff.text.left(diff.text.size() - 1);
    }
    if (removeNewLineFromLastInsert) {
        Diff &diff = diffList[lastInsert];
        diff.text = diff.text.left(diff.text.size() - 1);
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

static QStringView readLine(QStringView text, QStringView *remainingText, bool *hasNewLine)
{
    const QChar newLine('\n');
    const int indexOfFirstNewLine = text.indexOf(newLine);
    if (indexOfFirstNewLine < 0) {
        if (remainingText)
            *remainingText = {};
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

static bool detectChunkData(QStringView chunkDiff, ChunkData *chunkData, QStringView *remainingPatch)
{
    bool hasNewLine;
    const QStringView chunkLine = readLine(chunkDiff, remainingPatch, &hasNewLine);

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
    QStringView leftPos = chunkLine.mid(leftPosStart, leftPosLength);

    const int rightPosStart = rightPosIndex + rightPosMarker.size();
    const int rightPosLength = optionalHintIndex - rightPosStart;
    QStringView rightPos = chunkLine.mid(rightPosStart, rightPosLength);

    const int optionalHintStart = optionalHintIndex + optionalHintMarker.size();
    const int optionalHintLength = chunkLine.size() - optionalHintStart;
    const QStringView optionalHint = chunkLine.mid(optionalHintStart, optionalHintLength);

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

    chunkData->startingLineNumber = {leftLineNumber - 1, rightLineNumber - 1};
    chunkData->contextInfo = optionalHint.toString();

    return true;
}

static QList<ChunkData> readChunks(QStringView patch, bool *lastChunkAtTheEndOfFile, bool *ok)
{
    QList<ChunkData> chunkDataList;
    int position = -1;

    QList<int> startingPositions; // store starting positions of @@
    if (patch.startsWith(QStringLiteral("@@ -")))
        startingPositions.append(position + 1);

    while ((position = patch.indexOf(QStringLiteral("\n@@ -"), position + 1)) >= 0)
        startingPositions.append(position + 1);

    const QChar newLine('\n');
    bool readOk = true;

    const int count = startingPositions.size();
    for (int i = 0; i < count; i++) {
        const int chunkStart = startingPositions.at(i);
        const int chunkEnd = (i < count - 1)
                                 // drop the newline before the next chunk start
                                 ? startingPositions.at(i + 1) - 1
                                 // drop the possible newline by the end of patch
                                 : (patch.at(patch.size() - 1) == newLine ? patch.size() - 1
                                                                          : patch.size());

        // extract just one chunk
        const QStringView chunkDiff = patch.mid(chunkStart, chunkEnd - chunkStart);

        ChunkData chunkData;
        QStringView lines;
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

static FileData readDiffHeaderAndChunks(QStringView headerAndChunks, bool *ok)
{
    QStringView patch = headerAndChunks;
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
        fileData.fileInfo[LeftSide].fileName = leftMatch.captured(1);

        // followed by rightFileRegExp
        const QRegularExpressionMatch rightMatch = rightFileRegExp.match(patch);
        if (rightMatch.hasMatch() && rightMatch.capturedStart() == 0) {
            patch = patch.mid(rightMatch.capturedEnd());
            fileData.fileInfo[RightSide].fileName = rightMatch.captured(1);

            fileData.chunks = readChunks(patch,
                                         &fileData.lastChunkAtTheEndOfFile,
                                         &readOk);
        }
    } else {
        // or by binaryRegExp
        const QRegularExpressionMatch binaryMatch = binaryRegExp.match(patch);
        if (binaryMatch.hasMatch() && binaryMatch.capturedStart() == 0) {
            fileData.fileInfo[LeftSide].fileName = binaryMatch.captured(1);
            fileData.fileInfo[RightSide].fileName = binaryMatch.captured(2);
            fileData.binaryFiles = true;
            readOk = true;
        }
    }

    if (ok)
        *ok = readOk;

    if (!readOk)
        return {};

    return fileData;

}

static void readDiffPatch(QPromise<QList<FileData>> &promise, QStringView patch)
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
            if (promise.isCanceled())
                return;

            int pos = diffMatch.capturedStart();
            if (lastPos >= 0) {
                QStringView headerAndChunks = patch.mid(lastPos, pos - lastPos);

                const FileData fileData = readDiffHeaderAndChunks(headerAndChunks, &readOk);

                if (!readOk)
                    break;

                fileDataList.append(fileData);
            }
            lastPos = pos;
            pos = diffMatch.capturedEnd();
            diffMatch = diffRegExp.match(patch, pos);
        } while (diffMatch.hasMatch());

        if (readOk) {
            QStringView headerAndChunks = patch.mid(lastPos, patch.size() - lastPos - 1);

            const FileData fileData = readDiffHeaderAndChunks(headerAndChunks, &readOk);

            if (readOk)
                fileDataList.append(fileData);
        }
    }
    if (!readOk)
        return;
    promise.addResult(fileDataList);
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
// 2a. old mode [oldFileModeNumber]\n
//     new mode [newFileModeNumber]\n
// 2b. <Nothing, only in case when no ChangeMode>
// 3.  [dis]similarity index [0-100]%\n
//     [copy / rename] from [leftFileName]\n
//     [copy / rename] to [rightFileName]
// 4a. <Nothing more, only when similarity index was 100%>
// 4b. index [leftIndexSha]..[rightIndexSha] <optionally: octalNumber>
// 5.  --- [leftFileNameOrDevNull]\n
//     +++ [rightFileNameOrDevNull]\n
//     <Chunks>

static bool detectIndexAndBinary(QStringView patch, FileData *fileData, QStringView *remainingPatch)
{
    bool hasNewLine;
    *remainingPatch = patch;

    if (remainingPatch->isEmpty()) {
        switch (fileData->fileOperation) {
        case FileData::CopyFile:
        case FileData::RenameFile:
        case FileData::ChangeMode:
            // in case of 100% similarity we don't have more lines in the patch
            return true;
        default:
            break;
        }
    }

    QStringView afterNextLine;
    // index [leftIndexSha]..[rightIndexSha] <optionally: octalNumber>
    const QStringView nextLine = readLine(patch, &afterNextLine, &hasNewLine);

    const QLatin1String indexHeader("index ");

    if (nextLine.startsWith(indexHeader)) {
        const QStringView indices = nextLine.mid(indexHeader.size());
        const int dotsPosition = indices.indexOf(QStringLiteral(".."));
        if (dotsPosition < 0)
            return false;
        fileData->fileInfo[LeftSide].typeInfo = indices.left(dotsPosition).toString();

        // if there is no space we take the remaining string
        const int spacePosition = indices.indexOf(QChar::Space, dotsPosition + 2);
        const int length = spacePosition < 0 ? -1 : spacePosition - dotsPosition - 2;
        fileData->fileInfo[RightSide].typeInfo = indices.mid(dotsPosition + 2, length).toString();

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

    const QString leftFileName = sideFileName(LeftSide, *fileData);
    const QString rightFileName = sideFileName(RightSide, *fileData);
    const QString binaryLine = "Binary files "
            + leftFileName + " and "
            + rightFileName + " differ";

    if (*remainingPatch == binaryLine) {
        fileData->binaryFiles = true;
        *remainingPatch = {};
        return true;
    }

    const QString leftStart = "--- " + leftFileName;
    QStringView afterMinuses;
    // --- leftFileName
    const QStringView minuses = readLine(*remainingPatch, &afterMinuses, &hasNewLine);
    if (!hasNewLine)
        return false; // we need to have at least one more line

    if (!minuses.startsWith(leftStart))
        return false;

    const QString rightStart = "+++ " + rightFileName;
    QStringView afterPluses;
    // +++ rightFileName
    const QStringView pluses = readLine(afterMinuses, &afterPluses, &hasNewLine);
    if (!hasNewLine)
        return false; // we need to have at least one more line

    if (!pluses.startsWith(rightStart))
        return false;

    *remainingPatch = afterPluses;
    return true;
}

static bool extractCommonFileName(QStringView fileNames, QStringView *fileName)
{
    // we should have 1 space between filenames
    if (fileNames.size() % 2 == 0)
        return false;

    if (!fileNames.startsWith(QLatin1String("a/")))
        return false;

    // drop the space in between
    const int fileNameSize = fileNames.size() / 2;
    if (!fileNames.mid(fileNameSize).startsWith(QLatin1String(" b/")))
        return false;

    // drop "a/"
    const QStringView leftFileName = fileNames.mid(2, fileNameSize - 2);

    // drop the first filename + " b/"
    const QStringView rightFileName = fileNames.mid(fileNameSize + 3, fileNameSize - 2);

    if (leftFileName != rightFileName)
        return false;

    *fileName = leftFileName;
    return true;
}

static bool detectFileData(QStringView patch, FileData *fileData, QStringView *remainingPatch)
{
    bool hasNewLine;

    QStringView afterDiffGit;
    // diff --git a/leftFileName b/rightFileName
    const QStringView diffGit = readLine(patch, &afterDiffGit, &hasNewLine);
    if (!hasNewLine)
        return false; // we need to have at least one more line

    const QLatin1String gitHeader("diff --git ");
    const QStringView fileNames = diffGit.mid(gitHeader.size());
    QStringView commonFileName;
    if (extractCommonFileName(fileNames, &commonFileName)) {
        // change / new / delete

        fileData->fileOperation = FileData::ChangeFile;
        fileData->fileInfo[LeftSide].fileName = fileData->fileInfo[RightSide].fileName = commonFileName.toString();

        QStringView afterSecondLine;
        const QStringView secondLine = readLine(afterDiffGit, &afterSecondLine, &hasNewLine);

        if (secondLine.startsWith(QStringLiteral("new file mode "))) {
            fileData->fileOperation = FileData::NewFile;
            *remainingPatch = afterSecondLine;
        } else if (secondLine.startsWith(QStringLiteral("deleted file mode "))) {
            fileData->fileOperation = FileData::DeleteFile;
            *remainingPatch = afterSecondLine;
        } else if (secondLine.startsWith(QStringLiteral("old mode "))) {
            QStringView afterThirdLine;
            // new mode
            readLine(afterSecondLine, &afterThirdLine, &hasNewLine);
            if (!hasNewLine)
                fileData->fileOperation = FileData::ChangeMode;

            // TODO: validate new mode line
            *remainingPatch = afterThirdLine;
        } else {
            *remainingPatch = afterDiffGit;
        }

    } else {
        // copy / rename
        QStringView afterModeOrSimilarity;
        QStringView afterSimilarity;
        const QStringView secondLine = readLine(afterDiffGit, &afterModeOrSimilarity, &hasNewLine);
        if (secondLine.startsWith(QLatin1String("old mode "))) {
            if (!hasNewLine)
                return false;
            readLine(afterModeOrSimilarity, &afterModeOrSimilarity, &hasNewLine); // new mode
            if (!hasNewLine)
                return false;
            // (dis)similarity index [0-100]%
            readLine(afterModeOrSimilarity, &afterSimilarity, &hasNewLine);
        } else {
            afterSimilarity = afterModeOrSimilarity;
        }

        if (!hasNewLine)
            return false; // we need to have at least one more line

        // TODO: validate similarity line

        QStringView afterCopyRenameFrom;
        // [copy / rename] from leftFileName
        const QStringView copyRenameFrom = readLine(afterSimilarity, &afterCopyRenameFrom, &hasNewLine);
        if (!hasNewLine)
            return false; // we need to have at least one more line

        const QLatin1String copyFrom("copy from ");
        const QLatin1String renameFrom("rename from ");
        if (copyRenameFrom.startsWith(copyFrom)) {
            fileData->fileOperation = FileData::CopyFile;
            fileData->fileInfo[LeftSide].fileName = copyRenameFrom.mid(copyFrom.size()).toString();
        } else if (copyRenameFrom.startsWith(renameFrom)) {
            fileData->fileOperation = FileData::RenameFile;
            fileData->fileInfo[LeftSide].fileName = copyRenameFrom.mid(renameFrom.size()).toString();
        } else {
            return false;
        }

        QStringView afterCopyRenameTo;
        // [copy / rename] to rightFileName
        const QStringView copyRenameTo = readLine(afterCopyRenameFrom, &afterCopyRenameTo, &hasNewLine);

        // if (dis)similarity index is 100% we don't have more lines

        const QLatin1String copyTo("copy to ");
        const QLatin1String renameTo("rename to ");
        if (fileData->fileOperation == FileData::CopyFile && copyRenameTo.startsWith(copyTo)) {
            fileData->fileInfo[RightSide].fileName = copyRenameTo.mid(copyTo.size()).toString();
        } else if (fileData->fileOperation == FileData::RenameFile && copyRenameTo.startsWith(renameTo)) {
            fileData->fileInfo[RightSide].fileName = copyRenameTo.mid(renameTo.size()).toString();
        } else {
            return false;
        }

        *remainingPatch = afterCopyRenameTo;
    }
    return detectIndexAndBinary(*remainingPatch, fileData, remainingPatch);
}

static void readGitPatch(QPromise<QList<FileData>> &promise, QStringView patch)
{
    int position = -1;

    QList<int> startingPositions; // store starting positions of git headers
    if (patch.startsWith(QStringLiteral("diff --git ")))
        startingPositions.append(position + 1);

    while ((position = patch.indexOf(QStringLiteral("\ndiff --git "), position + 1)) >= 0)
        startingPositions.append(position + 1);

    class PatchInfo {
    public:
        QStringView patch;
        FileData fileData;
    };

    const QChar newLine('\n');

    QList<PatchInfo> patches;
    const int count = startingPositions.size();
    for (int i = 0; i < count; i++) {
        if (promise.isCanceled())
            return;

        const int diffStart = startingPositions.at(i);
        const int diffEnd = (i < count - 1)
                                // drop the newline before the next header start
                                ? startingPositions.at(i + 1) - 1
                                // drop the possible newline by the end of file
                                : (patch.at(patch.size() - 1) == newLine ? patch.size() - 1
                                                                         : patch.size());

        // extract the patch for just one file
        const QStringView fileDiff = patch.mid(diffStart, diffEnd - diffStart);

        FileData fileData;
        QStringView remainingFileDiff;
        if (!detectFileData(fileDiff, &fileData, &remainingFileDiff))
            return;

        patches.append(PatchInfo { remainingFileDiff, fileData });
    }

    if (patches.isEmpty())
        return;

    promise.setProgressRange(0, patches.size());

    QList<FileData> fileDataList;
    int i = 0;
    for (const auto &patchInfo : std::as_const(patches)) {
        if (promise.isCanceled())
            return;
        promise.setProgressValue(i++);

        FileData fileData = patchInfo.fileData;
        bool readOk = false;
        if (!patchInfo.patch.isEmpty() || fileData.fileOperation == FileData::ChangeFile)
            fileData.chunks = readChunks(patchInfo.patch, &fileData.lastChunkAtTheEndOfFile, &readOk);
        else
            readOk = true;

        if (!readOk)
            return;

        fileDataList.append(fileData);
    }
    promise.addResult(fileDataList);
}

std::optional<QList<FileData>> DiffUtils::readPatch(const QString &patch)
{
    QPromise<QList<FileData>> promise;
    promise.start();
    readPatchWithPromise(promise, patch);
    if (promise.future().resultCount() == 0)
        return {};
    return promise.future().result();
}

void DiffUtils::readPatchWithPromise(QPromise<QList<FileData>> &promise, const QString &patch)
{
    promise.setProgressRange(0, 1);
    promise.setProgressValue(0);
    QStringView croppedPatch = QStringView(patch);
    // Crop e.g. "-- \n2.10.2.windows.1\n\n" at end of file
    const QRegularExpression formatPatchEndingRegExp("(\\n-- \\n\\S*\\n\\n$)");
    const QRegularExpressionMatch match = formatPatchEndingRegExp.match(croppedPatch);
    if (match.hasMatch())
        croppedPatch = croppedPatch.left(match.capturedStart() + 1);

    readGitPatch(promise, croppedPatch);
    if (promise.future().resultCount() == 0)
        readDiffPatch(promise, croppedPatch);
}

} // namespace DiffEditor
