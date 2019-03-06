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

#include "textutils.h"

#include <QTextDocument>
#include <QTextBlock>

namespace Utils {
namespace Text {

bool convertPosition(const QTextDocument *document, int pos, int *line, int *column)
{
    QTextBlock block = document->findBlock(pos);
    if (!block.isValid()) {
        (*line) = -1;
        (*column) = -1;
        return false;
    } else {
        // line and column are both 1-based
        (*line) = block.blockNumber() + 1;
        (*column) = pos - block.position() + 1;
        return true;
    }
}

OptionalLineColumn convertPosition(const QTextDocument *document, int pos)
{
    OptionalLineColumn optional;

    QTextBlock block = document->findBlock(pos);

    if (block.isValid())
        optional.emplace(block.blockNumber() + 1, pos - block.position() + 1);

    return optional;
}

int positionInText(const QTextDocument *textDocument, int line, int column)
{
    // Deduct 1 from line and column since they are 1-based.
    // Column should already be converted from UTF-8 byte offset to the TextEditor column.
    return textDocument->findBlockByNumber(line - 1).position() + column - 1;
}

QString textAt(QTextCursor tc, int pos, int length)
{
    if (pos < 0)
        pos = 0;
    tc.movePosition(QTextCursor::End);
    if (pos + length > tc.position())
        length = tc.position() - pos;

    tc.setPosition(pos);
    tc.setPosition(pos + length, QTextCursor::KeepAnchor);

    // selectedText() returns U+2029 (PARAGRAPH SEPARATOR) instead of newline
    return tc.selectedText().replace(QChar::ParagraphSeparator, QLatin1Char('\n'));
}

QTextCursor selectAt(QTextCursor textCursor, uint line, uint column, uint length)
{
    if (line < 1)
        line = 1;

    if (column < 1)
        column = 1;

    textCursor.setPosition(0);
    textCursor.movePosition(QTextCursor::NextBlock, QTextCursor::MoveAnchor, line - 1);
    textCursor.movePosition(QTextCursor::NextCharacter,QTextCursor::MoveAnchor, column  + length - 1 );

    textCursor.movePosition(QTextCursor::PreviousCharacter,QTextCursor::KeepAnchor, length);

    return textCursor;
}

QTextCursor flippedCursor(const QTextCursor &cursor)
{
    QTextCursor flipped = cursor;
    flipped.clearSelection();
    flipped.setPosition(cursor.anchor(), QTextCursor::KeepAnchor);
    return flipped;
}

static bool isValidIdentifierChar(const QChar &c)
{
    return c.isLetter()
        || c.isNumber()
        || c == QLatin1Char('_')
        || c.isHighSurrogate()
        || c.isLowSurrogate();
}

static bool isAfterOperatorKeyword(QTextCursor cursor)
{
    cursor.movePosition(QTextCursor::PreviousWord);
    cursor.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
    return cursor.selectedText() == "operator";
}

QTextCursor wordStartCursor(const QTextCursor &textCursor)
{
    const int originalPosition = textCursor.position();
    QTextCursor cursor(textCursor);
    cursor.movePosition(QTextCursor::StartOfWord);
    const int wordStartPosition = cursor.position();

    if (originalPosition == wordStartPosition) {
        // Cursor is not on an identifier, check whether we are right after one.
        const QChar c = textCursor.document()->characterAt(originalPosition - 1);
        if (isValidIdentifierChar(c))
            cursor.movePosition(QTextCursor::PreviousWord);
    }
    if (isAfterOperatorKeyword(cursor))
        cursor.movePosition(QTextCursor::PreviousWord);

    return cursor;
}

QString wordUnderCursor(const QTextCursor &cursor)
{
    QTextCursor tc(cursor);
    tc.select(QTextCursor::WordUnderCursor);
    return tc.selectedText();
}

int utf8NthLineOffset(const QTextDocument *textDocument, const QByteArray &buffer, int line)
{
    if (textDocument->blockCount() < line)
        return -1;

    if (textDocument->characterCount() == buffer.size() + 1)
        return textDocument->findBlockByNumber(line - 1).position();

    int utf8Offset = 0;
    for (int count = 0; count < line - 1; ++count) {
        utf8Offset = buffer.indexOf('\n', utf8Offset);
        if (utf8Offset == -1)
            return -1; // The line does not exist.
        ++utf8Offset;
    }
    return utf8Offset;
}

LineColumn utf16LineColumn(const QByteArray &utf8Buffer, int utf8Offset)
{
    Utils::LineColumn lineColumn;
    lineColumn.line = static_cast<int>(
                          std::count(utf8Buffer.begin(), utf8Buffer.begin() + utf8Offset, '\n'))
                      + 1;
    const int startOfLineOffset = utf8Offset ? (utf8Buffer.lastIndexOf('\n', utf8Offset - 1) + 1)
                                             : 0;
    lineColumn.column = QString::fromUtf8(
                            utf8Buffer.mid(startOfLineOffset, utf8Offset - startOfLineOffset))
                            .length()
                        + 1;
    return lineColumn;
}

QString utf16LineTextInUtf8Buffer(const QByteArray &utf8Buffer, int currentUtf8Offset)
{
    const int lineStartUtf8Offset = currentUtf8Offset
                                        ? (utf8Buffer.lastIndexOf('\n', currentUtf8Offset - 1) + 1)
                                        : 0;
    const int lineEndUtf8Offset = utf8Buffer.indexOf('\n', currentUtf8Offset);
    return QString::fromUtf8(
        utf8Buffer.mid(lineStartUtf8Offset, lineEndUtf8Offset - lineStartUtf8Offset));
}

} // Text
} // Utils
