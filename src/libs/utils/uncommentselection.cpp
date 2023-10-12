// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "uncommentselection.h"

#include "qtcassert.h"
#include "multitextcursor.h"

#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>

namespace Utils {

CommentDefinition CommentDefinition::CppStyle = CommentDefinition("//", "/*", "*/");
CommentDefinition CommentDefinition::HashStyle = CommentDefinition("#");

CommentDefinition::CommentDefinition() = default;

CommentDefinition::CommentDefinition(const QString &single, const QString &multiStart,
                                     const QString &multiEnd)
    : singleLine(single),
      multiLineStart(multiStart),
      multiLineEnd(multiEnd)
{
}

bool CommentDefinition::isValid() const
{
    return hasSingleLineStyle() || hasMultiLineStyle();
}

bool CommentDefinition::hasSingleLineStyle() const
{
    return !singleLine.isEmpty();
}

bool CommentDefinition::hasMultiLineStyle() const
{
    return !multiLineStart.isEmpty() && !multiLineEnd.isEmpty();
}

static bool isComment(const QString &text, int index, const QString &commentType)
{
    return QStringView(text).mid(index).startsWith(commentType);
}

QTextCursor unCommentSelection(const QTextCursor &cursorIn,
                               const CommentDefinition &definition,
                               bool preferSingleLine)
{
    if (!definition.isValid())
        return cursorIn;

    QTextCursor cursor = cursorIn;
    QTextDocument *doc = cursor.document();
    cursor.beginEditBlock();

    int pos = cursor.position();
    int anchor = cursor.anchor();
    int start = qMin(anchor, pos);
    int end = qMax(anchor, pos);
    bool anchorIsStart = (anchor == start);

    QTextBlock startBlock = doc->findBlock(start);
    QTextBlock endBlock = doc->findBlock(end);

    if (end > start && endBlock.position() == end) {
        --end;
        endBlock = endBlock.previous();
    }

    bool doMultiLineStyleUncomment = false;
    bool doMultiLineStyleComment = false;
    bool doSingleLineStyleUncomment = false;

    bool hasSelection = cursor.hasSelection();

    if (hasSelection && definition.hasMultiLineStyle() && !preferSingleLine) {

        QString startText = startBlock.text();
        int startPos = start - startBlock.position();
        const int multiLineStartLength = definition.multiLineStart.length();
        bool hasLeadingCharacters = !startText.left(startPos).trimmed().isEmpty();

        if (startPos >= multiLineStartLength
            && isComment(startText,
                         startPos - multiLineStartLength,
                         definition.multiLineStart)) {
            startPos -= multiLineStartLength;
            start -= multiLineStartLength;
        }

        bool hasSelStart = startPos <= startText.length() - multiLineStartLength
            && isComment(startText, startPos, definition.multiLineStart);

        QString endText = endBlock.text();
        int endPos = end - endBlock.position();
        const int multiLineEndLength = definition.multiLineEnd.length();
        bool hasTrailingCharacters =
                !endText.left(endPos).remove(definition.singleLine).trimmed().isEmpty()
                && !endText.mid(endPos).trimmed().isEmpty();

        if (endPos <= endText.length() - multiLineEndLength
            && isComment(endText, endPos, definition.multiLineEnd)) {
            endPos += multiLineEndLength;
            end += multiLineEndLength;
        }

        bool hasSelEnd = endPos >= multiLineEndLength
            && isComment(endText, endPos - multiLineEndLength, definition.multiLineEnd);

        doMultiLineStyleUncomment = hasSelStart && hasSelEnd;
        doMultiLineStyleComment = !doMultiLineStyleUncomment
                                  && (hasLeadingCharacters
                                      || hasTrailingCharacters
                                      || !definition.hasSingleLineStyle());
    } else if (!hasSelection && !definition.hasSingleLineStyle()) {

        QString text = startBlock.text().trimmed();

        doMultiLineStyleUncomment = text.startsWith(definition.multiLineStart)
                                    && text.endsWith(definition.multiLineEnd);
        doMultiLineStyleComment = !doMultiLineStyleUncomment && !text.isEmpty();

        start = startBlock.position();
        end = endBlock.position() + endBlock.length() - 1;

        if (doMultiLineStyleUncomment) {
            int offset = 0;
            text = startBlock.text();
            const int length = text.length();
            while (offset < length && text.at(offset).isSpace())
                ++offset;
            start += offset;
        }
    }

    if (doMultiLineStyleUncomment) {
        cursor.setPosition(end);
        cursor.movePosition(QTextCursor::PreviousCharacter,
                            QTextCursor::KeepAnchor,
                            definition.multiLineEnd.length());
        cursor.removeSelectedText();
        cursor.setPosition(start);
        cursor.movePosition(QTextCursor::NextCharacter,
                            QTextCursor::KeepAnchor,
                            definition.multiLineStart.length());
        cursor.removeSelectedText();
    } else if (doMultiLineStyleComment) {
        cursor.setPosition(end);
        cursor.insertText(definition.multiLineEnd);
        cursor.setPosition(start);
        cursor.insertText(definition.multiLineStart);
    } else {
        endBlock = endBlock.next();
        doSingleLineStyleUncomment = true;
        for (QTextBlock block = startBlock; block != endBlock; block = block.next()) {
            QString text = block.text().trimmed();
            if (!text.isEmpty() && !text.startsWith(definition.singleLine)) {
                doSingleLineStyleUncomment = false;
                break;
            }
        }

        const int singleLineLength = definition.singleLine.length();
        int minTab = INT_MAX;
        if (definition.isAfterWhitespace && !doSingleLineStyleUncomment) {
            for (QTextBlock block = startBlock; block != endBlock && minTab != 0; block = block.next()) {
                QTextCursor c(block);
                if (doc->characterAt(block.position()).isSpace()) {
                    c.movePosition(QTextCursor::NextWord);
                    if (c.block() != block) // ignore empty lines
                        continue;
                }
                const int pos = c.positionInBlock();
                if (pos < minTab)
                    minTab = pos;
            }
        }
        for (QTextBlock block = startBlock; block != endBlock; block = block.next()) {
            if (doSingleLineStyleUncomment) {
                QString text = block.text();
                int i = 0;
                while (i <= text.size() - singleLineLength) {
                    if (definition.isAfterWhitespace
                        && isComment(text, i, definition.singleLine + ' ')) {
                        cursor.setPosition(block.position() + i);
                        cursor.movePosition(QTextCursor::NextCharacter,
                                            QTextCursor::KeepAnchor,
                                            singleLineLength + 1);
                        cursor.removeSelectedText();
                        break;
                    } else if (isComment(text, i, definition.singleLine)) {
                        cursor.setPosition(block.position() + i);
                        cursor.movePosition(QTextCursor::NextCharacter,
                                            QTextCursor::KeepAnchor,
                                            singleLineLength);
                        cursor.removeSelectedText();
                        break;
                    }
                    if (!text.at(i).isSpace())
                        break;
                    ++i;
                }
            } else {
                const QString text = block.text();
                for (QChar c : text) {
                    if (!c.isSpace()) {
                        if (definition.isAfterWhitespace) {
                            cursor.setPosition(block.position() + minTab);
                            cursor.insertText(definition.singleLine + ' ');
                        } else {
                            cursor.setPosition(block.position());
                            cursor.insertText(definition.singleLine);
                        }
                        break;
                    }
                }
            }
        }
    }

    cursor.endEditBlock();

    cursor = cursorIn;
    // adjust selection when commenting out
    if (hasSelection && !doMultiLineStyleUncomment && !doSingleLineStyleUncomment) {
        if (!doMultiLineStyleComment)
            start = startBlock.position(); // move the comment into the selection
        int lastSelPos = anchorIsStart ? cursor.position() : cursor.anchor();
        if (anchorIsStart) {
            cursor.setPosition(start);
            cursor.setPosition(lastSelPos, QTextCursor::KeepAnchor);
        } else {
            cursor.setPosition(lastSelPos);
            cursor.setPosition(start, QTextCursor::KeepAnchor);
        }
    }
    return cursor;
}

MultiTextCursor unCommentSelection(const MultiTextCursor &cursorIn,
                                   const CommentDefinition &definiton,
                                   bool preferSingleLine)
{
    if (cursorIn.isNull())
        return cursorIn;
    if (!cursorIn.hasMultipleCursors())
        return MultiTextCursor({unCommentSelection(cursorIn.mainCursor(), definiton, preferSingleLine)});
    QMap<int, QTextCursor> cursors;
    for (const QTextCursor &c : cursorIn) {
        QTextBlock block = c.document()->findBlock(c.selectionStart());
        QTC_ASSERT(block.isValid(), continue);
        QTextBlock end = c.document()->findBlock(c.selectionEnd());
        QTC_ASSERT(end.isValid(), continue);
        end = end.next();
        while (block != end && block.isValid()) {
            if (!cursors.contains(block.blockNumber()))
                cursors.insert(block.blockNumber(), QTextCursor(block));
            block = block.next();
        }
    }
    for (const QTextCursor &c : cursors)
        unCommentSelection(c, definiton, /*always prefer single line for multi cursor*/ true);
    return cursorIn;
}

} // namespace Utils
