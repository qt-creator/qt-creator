/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "multitextcursor.h"

#include "algorithm.h"
#include "camelcasecursor.h"
#include "qtcassert.h"

#include <QKeyEvent>
#include <QTextBlock>

namespace Utils {

MultiTextCursor::MultiTextCursor() {}

MultiTextCursor::MultiTextCursor(const QList<QTextCursor> &cursors)
    : m_cursors(cursors)
{
    mergeCursors();
}

void MultiTextCursor::addCursor(const QTextCursor &cursor)
{
    QTC_ASSERT(!cursor.isNull(), return);
    m_cursors.append(cursor);
    mergeCursors();
}

void MultiTextCursor::setCursors(const QList<QTextCursor> &cursors)
{
    m_cursors = cursors;
    mergeCursors();
}

const QList<QTextCursor> MultiTextCursor::cursors() const
{
    return m_cursors;
}

void MultiTextCursor::replaceMainCursor(const QTextCursor &cursor)
{
    QTC_ASSERT(!cursor.isNull(), return);
    takeMainCursor();
    addCursor(cursor);
}

QTextCursor MultiTextCursor::mainCursor() const
{
    if (m_cursors.isEmpty())
        return {};
    return m_cursors.last();
}

QTextCursor MultiTextCursor::takeMainCursor()
{
    if (m_cursors.isEmpty())
        return {};
    return m_cursors.takeLast();
}

void MultiTextCursor::beginEditBlock()
{
    QTC_ASSERT(!m_cursors.empty(), return);
    m_cursors.last().beginEditBlock();
}

void MultiTextCursor::endEditBlock()
{
    QTC_ASSERT(!m_cursors.empty(), return);
    m_cursors.last().endEditBlock();
}

bool MultiTextCursor::isNull() const
{
    return m_cursors.isEmpty();
}

bool MultiTextCursor::hasMultipleCursors() const
{
    return m_cursors.size() > 1;
}

int MultiTextCursor::cursorCount() const
{
    return m_cursors.size();
}

void MultiTextCursor::movePosition(QTextCursor::MoveOperation operation,
                                   QTextCursor::MoveMode mode,
                                   int n)
{
    for (QTextCursor &cursor : m_cursors)
        cursor.movePosition(operation, mode, n);
    mergeCursors();
}

bool MultiTextCursor::hasSelection() const
{
    return Utils::anyOf(m_cursors, &QTextCursor::hasSelection);
}

QString MultiTextCursor::selectedText() const
{
    QString text;
    QList<QTextCursor> cursors = m_cursors;
    Utils::sort(cursors);
    for (const QTextCursor &cursor : cursors) {
        const QString &cursorText = cursor.selectedText();
        if (cursorText.isEmpty())
            continue;
        if (!text.isEmpty())
            text.append('\n');
        text.append(cursorText);
    }
    return text;
}

void MultiTextCursor::removeSelectedText()
{
    beginEditBlock();
    for (QTextCursor &c : m_cursors)
        c.removeSelectedText();
    endEditBlock();
    mergeCursors();
}

static void insertAndSelect(QTextCursor &cursor, const QString &text, bool selectNewText)
{
    if (selectNewText) {
        const int anchor = cursor.position();
        cursor.insertText(text);
        const int pos = cursor.position();
        cursor.setPosition(anchor);
        cursor.setPosition(pos, QTextCursor::KeepAnchor);
    } else {
        cursor.insertText(text);
    }
}

void MultiTextCursor::insertText(const QString &text, bool selectNewText)
{
    if (m_cursors.isEmpty())
        return;
    m_cursors.last().beginEditBlock();
    if (hasMultipleCursors()) {
        QStringList lines = text.split('\n');
        if (!lines.isEmpty() && lines.last().isEmpty())
            lines.pop_back();
        int index = 0;
        if (lines.count() == m_cursors.count()) {
            QList<QTextCursor> cursors = m_cursors;
            Utils::sort(cursors);
            for (QTextCursor &cursor : cursors)
                insertAndSelect(cursor, lines.at(index++), selectNewText);
            m_cursors.last().endEditBlock();
            return;
        }
    }
    for (QTextCursor &cursor : m_cursors)
        insertAndSelect(cursor, text, selectNewText);
    m_cursors.last().endEditBlock();
}

bool equalCursors(const QTextCursor &lhs, const QTextCursor &rhs)
{
    return lhs == rhs && lhs.anchor() == rhs.anchor();
}

bool MultiTextCursor::operator==(const MultiTextCursor &other) const
{
    if (m_cursors.size() != other.m_cursors.size())
        return false;
    if (m_cursors.isEmpty())
        return true;
    QList<QTextCursor> thisCursors = m_cursors;
    QList<QTextCursor> otherCursors = other.m_cursors;
    if (!equalCursors(thisCursors.takeLast(), otherCursors.takeLast()))
        return false;
    for (const QTextCursor &oc : otherCursors) {
        auto compare = [oc](const QTextCursor &c) { return equalCursors(oc, c); };
        if (!Utils::contains(thisCursors, compare))
            return false;
    }
    return true;
}

bool MultiTextCursor::operator!=(const MultiTextCursor &other) const
{
    return !operator==(other);
}

static bool cursorsOverlap(const QTextCursor &c1, const QTextCursor &c2)
{
    if (c1.hasSelection()) {
        if (c2.hasSelection()) {
            return c2.selectionEnd() > c1.selectionStart()
                   && c2.selectionStart() < c1.selectionEnd();
        }
        const int c2Pos = c2.position();
        return c2Pos > c1.selectionStart() && c2Pos < c1.selectionEnd();
    }
    if (c2.hasSelection()) {
        const int c1Pos = c1.position();
        return c1Pos > c2.selectionStart() && c1Pos < c2.selectionEnd();
    }
    return c1 == c2;
};

static void mergeCursors(QTextCursor &c1, const QTextCursor &c2)
{
    if (c1.position() == c2.position() && c1.anchor() == c2.anchor())
        return;
    if (c1.hasSelection()) {
        if (!c2.hasSelection())
            return;
        int pos = c1.position();
        int anchor = c1.anchor();
        if (c1.selectionStart() > c2.selectionStart()) {
            if (pos < anchor)
                pos = c2.selectionStart();
            else
                anchor = c2.selectionStart();
        }
        if (c1.selectionEnd() < c2.selectionEnd()) {
            if (pos < anchor)
                anchor = c2.selectionEnd();
            else
                pos = c2.selectionEnd();
        }
        c1.setPosition(anchor);
        c1.setPosition(pos, QTextCursor::KeepAnchor);
    } else {
        c1 = c2;
    }
}

void MultiTextCursor::mergeCursors()
{
    std::list<QTextCursor> cursors(m_cursors.begin(), m_cursors.end());
    cursors = Utils::filtered(cursors, [](const QTextCursor &c){
        return !c.isNull();
    });
    for (auto it = cursors.begin(); it != cursors.end(); ++it) {
        QTextCursor &c1 = *it;
        for (auto other = std::next(it); other != cursors.end();) {
            const QTextCursor &c2 = *other;
            if (cursorsOverlap(c1, c2)) {
                Utils::mergeCursors(c1, c2);
                other = cursors.erase(other);
                continue;
            }
            ++other;
        }
    }
    m_cursors = QList<QTextCursor>(cursors.begin(), cursors.end());
}

// could go into QTextCursor...
static QTextLine currentTextLine(const QTextCursor &cursor)
{
    const QTextBlock block = cursor.block();
    if (!block.isValid())
        return {};

    const QTextLayout *layout = block.layout();
    if (!layout)
        return {};

    const int relativePos = cursor.position() - block.position();
    return layout->lineForTextPosition(relativePos);
}

bool multiCursorAddEvent(QKeyEvent *e, QKeySequence::StandardKey matchKey)
{
    uint searchkey = (e->modifiers() | e->key())
                     & ~(Qt::KeypadModifier | Qt::GroupSwitchModifier | Qt::AltModifier);

    const QList<QKeySequence> bindings = QKeySequence::keyBindings(matchKey);
    return bindings.contains(QKeySequence(searchkey));
}

bool MultiTextCursor::handleMoveKeyEvent(QKeyEvent *e,
                                         QPlainTextEdit *edit,
                                         bool camelCaseNavigationEnabled)
{
    if (e->modifiers() & Qt::AltModifier) {
        QTextCursor::MoveOperation op = QTextCursor::NoMove;
        if (multiCursorAddEvent(e, QKeySequence::MoveToNextWord)) {
            op = QTextCursor::WordRight;
        } else if (multiCursorAddEvent(e, QKeySequence::MoveToPreviousWord)) {
            op = QTextCursor::WordLeft;
        } else if (multiCursorAddEvent(e, QKeySequence::MoveToEndOfBlock)) {
            op = QTextCursor::EndOfBlock;
        } else if (multiCursorAddEvent(e, QKeySequence::MoveToStartOfBlock)) {
            op = QTextCursor::StartOfBlock;
        } else if (multiCursorAddEvent(e, QKeySequence::MoveToNextLine)) {
            op = QTextCursor::Down;
        } else if (multiCursorAddEvent(e, QKeySequence::MoveToPreviousLine)) {
            op = QTextCursor::Up;
        } else if (multiCursorAddEvent(e, QKeySequence::MoveToStartOfLine)) {
            op = QTextCursor::StartOfLine;
        } else if (multiCursorAddEvent(e, QKeySequence::MoveToEndOfLine)) {
            op = QTextCursor::EndOfLine;
        } else if (multiCursorAddEvent(e, QKeySequence::MoveToStartOfDocument)) {
            op = QTextCursor::Start;
        } else if (multiCursorAddEvent(e, QKeySequence::MoveToEndOfDocument)) {
            op = QTextCursor::End;
        } else {
            return false;
        }

        const QList<QTextCursor> cursors = m_cursors;
        for (QTextCursor cursor : cursors) {
            if (camelCaseNavigationEnabled && op == QTextCursor::WordRight)
                CamelCaseCursor::right(&cursor, edit, QTextCursor::MoveAnchor);
            else if (camelCaseNavigationEnabled && op == QTextCursor::WordLeft)
                CamelCaseCursor::left(&cursor, edit, QTextCursor::MoveAnchor);
            else
                cursor.movePosition(op, QTextCursor::MoveAnchor);
            m_cursors << cursor;
        }

        mergeCursors();
        return true;
    }

    for (QTextCursor &cursor : m_cursors) {
        QTextCursor::MoveMode mode = QTextCursor::MoveAnchor;
        QTextCursor::MoveOperation op = QTextCursor::NoMove;

        if (e == QKeySequence::MoveToNextChar) {
            op = QTextCursor::Right;
        } else if (e == QKeySequence::MoveToPreviousChar) {
            op = QTextCursor::Left;
        } else if (e == QKeySequence::SelectNextChar) {
            op = QTextCursor::Right;
            mode = QTextCursor::KeepAnchor;
        } else if (e == QKeySequence::SelectPreviousChar) {
            op = QTextCursor::Left;
            mode = QTextCursor::KeepAnchor;
        } else if (e == QKeySequence::SelectNextWord) {
            op = QTextCursor::WordRight;
            mode = QTextCursor::KeepAnchor;
        } else if (e == QKeySequence::SelectPreviousWord) {
            op = QTextCursor::WordLeft;
            mode = QTextCursor::KeepAnchor;
        } else if (e == QKeySequence::SelectStartOfLine) {
            op = QTextCursor::StartOfLine;
            mode = QTextCursor::KeepAnchor;
        } else if (e == QKeySequence::SelectEndOfLine) {
            op = QTextCursor::EndOfLine;
            mode = QTextCursor::KeepAnchor;
        } else if (e == QKeySequence::SelectStartOfBlock) {
            op = QTextCursor::StartOfBlock;
            mode = QTextCursor::KeepAnchor;
        } else if (e == QKeySequence::SelectEndOfBlock) {
            op = QTextCursor::EndOfBlock;
            mode = QTextCursor::KeepAnchor;
        } else if (e == QKeySequence::SelectStartOfDocument) {
            op = QTextCursor::Start;
            mode = QTextCursor::KeepAnchor;
        } else if (e == QKeySequence::SelectEndOfDocument) {
            op = QTextCursor::End;
            mode = QTextCursor::KeepAnchor;
        } else if (e == QKeySequence::SelectPreviousLine) {
            op = QTextCursor::Up;
            mode = QTextCursor::KeepAnchor;
        } else if (e == QKeySequence::SelectNextLine) {
            op = QTextCursor::Down;
            mode = QTextCursor::KeepAnchor;
            {
                QTextBlock block = cursor.block();
                QTextLine line = currentTextLine(cursor);
                if (!block.next().isValid() && line.isValid()
                    && line.lineNumber() == block.layout()->lineCount() - 1)
                    op = QTextCursor::End;
            }
        } else if (e == QKeySequence::MoveToNextWord) {
            op = QTextCursor::WordRight;
        } else if (e == QKeySequence::MoveToPreviousWord) {
            op = QTextCursor::WordLeft;
        } else if (e == QKeySequence::MoveToEndOfBlock) {
            op = QTextCursor::EndOfBlock;
        } else if (e == QKeySequence::MoveToStartOfBlock) {
            op = QTextCursor::StartOfBlock;
        } else if (e == QKeySequence::MoveToNextLine) {
            op = QTextCursor::Down;
        } else if (e == QKeySequence::MoveToPreviousLine) {
            op = QTextCursor::Up;
        } else if (e == QKeySequence::MoveToStartOfLine) {
            op = QTextCursor::StartOfLine;
        } else if (e == QKeySequence::MoveToEndOfLine) {
            op = QTextCursor::EndOfLine;
        } else if (e == QKeySequence::MoveToStartOfDocument) {
            op = QTextCursor::Start;
        } else if (e == QKeySequence::MoveToEndOfDocument) {
            op = QTextCursor::End;
        } else {
            return false;
        }

        // Except for pageup and pagedown, macOS has very different behavior, we don't do it all, but
        // here's the breakdown:
        // Shift still works as an anchor, but only one of the other keys can be down Ctrl (Command),
        // Alt (Option), or Meta (Control).
        // Command/Control + Left/Right -- Move to left or right of the line
        //                 + Up/Down -- Move to top bottom of the file. (Control doesn't move the cursor)
        // Option + Left/Right -- Move one word Left/right.
        //        + Up/Down  -- Begin/End of Paragraph.
        // Home/End Top/Bottom of file. (usually don't move the cursor, but will select)

        bool visualNavigation = cursor.visualNavigation();
        cursor.setVisualNavigation(true);

        if (camelCaseNavigationEnabled && op == QTextCursor::WordRight)
            CamelCaseCursor::right(&cursor, edit, mode);
        else if (camelCaseNavigationEnabled && op == QTextCursor::WordLeft)
            CamelCaseCursor::left(&cursor, edit, mode);
        else if (!cursor.movePosition(op, mode) && mode == QTextCursor::MoveAnchor)
            cursor.clearSelection();
        cursor.setVisualNavigation(visualNavigation);
    }
    mergeCursors();
    return true;
}

} // namespace Utils
