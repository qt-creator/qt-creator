// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "textutils.h"
#include "qtcassert.h"

#include <QRegularExpression>
#include <QTextBlock>
#include <QTextDocument>

namespace Utils::Text {

bool Position::operator==(const Position &other) const
{
    return line == other.line && column == other.column;
}

/*!
    Returns the text position of a \a fileName and sets the \a postfixPos if
    it can find a positional postfix.

    The following patterns are supported: \c {filepath.txt:19},
    \c{filepath.txt:19:12}, \c {filepath.txt+19},
    \c {filepath.txt+19+12}, and \c {filepath.txt(19)}.
*/

Position Position::fromFileName(QStringView fileName, int &postfixPos)
{
    static const auto regexp = QRegularExpression("[:+](\\d+)?([:+](\\d+)?)?$");
    // (10) MSVC-style
    static const auto vsRegexp = QRegularExpression("[(]((\\d+)[)]?)?$");
    const QRegularExpressionMatch match = regexp.match(fileName);
    Position pos;
    if (match.hasMatch()) {
        postfixPos = match.capturedStart(0);
        if (match.lastCapturedIndex() > 0) {
            pos.line = match.captured(1).toInt();
            if (match.lastCapturedIndex() > 2) // index 2 includes the + or : for the column number
                pos.column = match.captured(3).toInt() - 1; //column is 0 based, despite line being 1 based
        }
    } else {
        const QRegularExpressionMatch vsMatch = vsRegexp.match(fileName);
        postfixPos = vsMatch.capturedStart(0);
        if (vsMatch.lastCapturedIndex() > 1) // index 1 includes closing )
            pos.line = vsMatch.captured(2).toInt();
    }
    if (pos.line > 0 && pos.column < 0)
        pos.column = 0; // if we got a valid line make sure to return a valid TextPosition
    return pos;
}

Position Position::fromPositionInDocument(const QTextDocument *document, int pos)
{
    QTC_ASSERT(document, return {});
    const QTextBlock block = document->findBlock(pos);
    if (block.isValid())
        return {block.blockNumber() + 1, pos - block.position()};

    return {};
}

Position Position::fromCursor(const QTextCursor &c)
{
    return c.isNull() ? Position{} : Position{c.blockNumber() + 1, c.positionInBlock()};
}

int Position::toPositionInDocument(const QTextDocument *document) const
{
    QTC_ASSERT(document, return -1);
    const QTextBlock block = document->findBlockByNumber(line - 1);
    if (block.isValid())
        return block.position() + qMin(column, block.length() - 1);

    return -1;
}

int Range::length(const QString &text) const
{
    if (end.line < begin.line)
        return -1;

    if (begin.line == end.line)
        return end.column - begin.column;

    int index = 0;
    int currentLine = 1;
    while (currentLine < begin.line) {
        index = text.indexOf(QChar::LineFeed, index);
        if (index < 0)
            return -1;
        ++index;
        ++currentLine;
    }
    const int beginIndex = index + begin.column;
    while (currentLine < end.line) {
        index = text.indexOf(QChar::LineFeed, index);
        if (index < 0)
            return -1;
        ++index;
        ++currentLine;
    }
    return index + end.column - beginIndex;
}

bool Range::operator==(const Range &other) const
{
    return begin == other.begin && end == other.end;
}

bool convertPosition(const QTextDocument *document, int pos, int *line, int *column)
{
    QTextBlock block = document->findBlock(pos);
    if (!block.isValid()) {
        (*line) = -1;
        (*column) = 0;
        return false;
    } else {
        // line and column are both 1-based
        (*line) = block.blockNumber() + 1;
        (*column) = pos - block.position();
        return true;
    }
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
    const int end = std::min(pos + length, tc.position());

    tc.setPosition(pos);
    tc.setPosition(end, QTextCursor::KeepAnchor);

    // selectedText() returns U+2029 (PARAGRAPH SEPARATOR) instead of newline
    return tc.selectedText().replace(QChar::ParagraphSeparator, QLatin1Char('\n'));
}

QTextCursor selectAt(QTextCursor textCursor, int line, int column, uint length)
{
    if (line < 1)
        line = 1;

    if (column < 0)
        column = 0;

    const int anchorPosition = positionInText(textCursor.document(), line, column + 1);
    textCursor.setPosition(anchorPosition);
    textCursor.setPosition(anchorPosition + int(length), QTextCursor::KeepAnchor);

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

QString utf16LineTextInUtf8Buffer(const QByteArray &utf8Buffer, int currentUtf8Offset)
{
    const int lineStartUtf8Offset = currentUtf8Offset
                                        ? (utf8Buffer.lastIndexOf('\n', currentUtf8Offset - 1) + 1)
                                        : 0;
    const int lineEndUtf8Offset = utf8Buffer.indexOf('\n', currentUtf8Offset);
    return QString::fromUtf8(
        utf8Buffer.mid(lineStartUtf8Offset, lineEndUtf8Offset - lineStartUtf8Offset));
}

static bool isByteOfMultiByteCodePoint(unsigned char byte)
{
    return byte & 0x80; // Check if most significant bit is set
}

bool utf8AdvanceCodePoint(const char *&current)
{
    if (Q_UNLIKELY(*current == '\0'))
        return false;

    // Process multi-byte UTF-8 code point (non-latin1)
    if (Q_UNLIKELY(isByteOfMultiByteCodePoint(*current))) {
        unsigned trailingBytesCurrentCodePoint = 1;
        for (unsigned char c = (*current) << 2; isByteOfMultiByteCodePoint(c); c <<= 1)
            ++trailingBytesCurrentCodePoint;
        current += trailingBytesCurrentCodePoint + 1;

    // Process single-byte UTF-8 code point (latin1)
    } else {
        ++current;
    }

    return true;
}

QDebug &operator<<(QDebug &stream, const Position &pos)
{
    stream << "line: " << pos.line << ", column: " << pos.column;
    return stream;
}

} // namespace Utils::Text
