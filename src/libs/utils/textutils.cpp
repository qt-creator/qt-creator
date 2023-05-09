// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "textutils.h"

#include <QTextDocument>
#include <QTextBlock>

namespace Utils::Text {

bool Position::operator==(const Position &other) const
{
    return line == other.line && column == other.column;
}

int Range::length(const QString &text) const
{
    if (begin.line == end.line)
        return end.column - begin.column;

    const int lineCount = end.line - begin.line;
    int index = text.indexOf(QChar::LineFeed);
    int currentLine = 1;
    while (index > 0 && currentLine < lineCount) {
        ++index;
        index = text.indexOf(QChar::LineFeed, index);
        ++currentLine;
    }

    if (index < 0)
        return 0;

    return index - begin.column + end.column;
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
        optional.emplace(block.blockNumber() + 1, pos - block.position());

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

QTextCursor selectAt(QTextCursor textCursor, int line, int column, uint length)
{
    if (line < 1)
        line = 1;

    if (column < 1)
        column = 1;

    const int anchorPosition = positionInText(textCursor.document(), line, column);
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

LineColumn utf16LineColumn(const QByteArray &utf8Buffer, int utf8Offset)
{
    LineColumn lineColumn;
    lineColumn.line = static_cast<int>(
                          std::count(utf8Buffer.begin(), utf8Buffer.begin() + utf8Offset, '\n'))
                      + 1;
    const int startOfLineOffset = utf8Offset ? (utf8Buffer.lastIndexOf('\n', utf8Offset - 1) + 1)
                                             : 0;
    lineColumn.column = QString::fromUtf8(
                            utf8Buffer.mid(startOfLineOffset, utf8Offset - startOfLineOffset))
                            .length();
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

void applyReplacements(QTextDocument *doc, const Replacements &replacements)
{
    if (replacements.empty())
        return;

    int fullOffsetShift = 0;
    QTextCursor editCursor(doc);
    editCursor.beginEditBlock();
    for (const Text::Replacement &replacement : replacements) {
        editCursor.setPosition(replacement.offset + fullOffsetShift);
        editCursor.movePosition(QTextCursor::NextCharacter,
                                QTextCursor::KeepAnchor,
                                replacement.length);
        editCursor.removeSelectedText();
        editCursor.insertText(replacement.text);
        fullOffsetShift += replacement.text.length() - replacement.length;
    }
    editCursor.endEditBlock();
}

} // namespace Utils::Text
