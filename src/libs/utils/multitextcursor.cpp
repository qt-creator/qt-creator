// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "multitextcursor.h"

#include "algorithm.h"
#include "camelcasecursor.h"
#include "hostosinfo.h"
#include "qtcassert.h"

#include <QKeyEvent>
#include <QTextBlock>

namespace Utils {

MultiTextCursor::MultiTextCursor() {}

MultiTextCursor::MultiTextCursor(const QList<QTextCursor> &cursors)
{
    setCursors(cursors);
}

void MultiTextCursor::fillMapWithList()
{
    m_cursorMap.clear();
    for (auto it = m_cursorList.begin(); it != m_cursorList.end(); ++it)
        m_cursorMap[it->selectionStart()] = it;
}

MultiTextCursor& MultiTextCursor::operator=(const MultiTextCursor &multiCursor)
{
    m_cursorList = multiCursor.m_cursorList;
    fillMapWithList();
    return *this;
}

MultiTextCursor::MultiTextCursor(const MultiTextCursor &multiCursor)
{
    *this = multiCursor;
}

MultiTextCursor& MultiTextCursor::operator=(const MultiTextCursor &&multiCursor)
{
    m_cursorList = std::move(multiCursor.m_cursorList);
    fillMapWithList();
    return *this;
}

MultiTextCursor::MultiTextCursor(const MultiTextCursor &&multiCursor)
{
    *this = std::move(multiCursor);
}

MultiTextCursor::~MultiTextCursor() = default;

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

void MultiTextCursor::addCursor(const QTextCursor &cursor)
{
    QTC_ASSERT(!cursor.isNull(), return);

    QTextCursor c1 = cursor;
    const int pos = c1.selectionStart();

    auto found = m_cursorMap.lower_bound(pos);
    if (found != m_cursorMap.begin())
        --found;

    for (; !m_cursorMap.empty() && found != m_cursorMap.end()
           && found->second->selectionStart() <= cursor.selectionEnd();) {
        const QTextCursor &c2 = *found->second;
        if (cursorsOverlap(c1, c2)) {
            Utils::mergeCursors(c1, c2);
            m_cursorList.erase(found->second);
            found = m_cursorMap.erase(found);
            continue;
        }
        ++found;
    }

    m_cursorMap[pos] = m_cursorList.insert(m_cursorList.end(), c1);
}

void MultiTextCursor::addCursors(const QList<QTextCursor> &cursors)
{
    for (const QTextCursor &c : cursors)
        addCursor(c);
}

void MultiTextCursor::setCursors(const QList<QTextCursor> &cursors)
{
    m_cursorList.clear();
    m_cursorMap.clear();
    addCursors(cursors);
}

const QList<QTextCursor> MultiTextCursor::cursors() const
{
    return QList<QTextCursor>(m_cursorList.begin(), m_cursorList.end());
}

void MultiTextCursor::replaceMainCursor(const QTextCursor &cursor)
{
    QTC_ASSERT(!cursor.isNull(), return);
    takeMainCursor();
    addCursor(cursor);
}

QTextCursor MultiTextCursor::mainCursor() const
{
    if (m_cursorList.empty())
        return {};
    return m_cursorList.back();
}

QTextCursor MultiTextCursor::takeMainCursor()
{
    if (m_cursorList.empty())
        return {};

    QTextCursor cursor = m_cursorList.back();
    auto it = m_cursorList.end();
    --it;
    m_cursorMap.erase(it->selectionStart());
    m_cursorList.erase(it);

    return cursor;
}

void MultiTextCursor::beginEditBlock()
{
    QTC_ASSERT(!m_cursorList.empty(), return);
    m_cursorList.back().beginEditBlock();
}

void MultiTextCursor::endEditBlock()
{
    QTC_ASSERT(!m_cursorList.empty(), return);
    m_cursorList.back().endEditBlock();
}

bool MultiTextCursor::isNull() const
{
    return m_cursorList.empty();
}

bool MultiTextCursor::hasMultipleCursors() const
{
    return m_cursorList.size() > 1;
}

int MultiTextCursor::cursorCount() const
{
    return static_cast<int>(m_cursorList.size());
}

void MultiTextCursor::movePosition(QTextCursor::MoveOperation operation,
                                   QTextCursor::MoveMode mode,
                                   int n)
{
    for (auto &cursor : m_cursorList)
        cursor.movePosition(operation, mode, n);

    mergeCursors();
}

bool MultiTextCursor::hasSelection() const
{
    return Utils::anyOf(m_cursorList, &QTextCursor::hasSelection);
}

QString MultiTextCursor::selectedText() const
{
    QString text;
    for (const auto &element : std::as_const(m_cursorMap)) {
        const QTextCursor &cursor = *element.second;
        const QString &cursorText = cursor.selectedText();
        if (cursorText.isEmpty())
            continue;
        if (!text.isEmpty()) {
            if (text.endsWith(QChar::ParagraphSeparator))
                text.chop(1);
            text.append('\n');
        }
        text.append(cursorText);
    }
    return text;
}

void MultiTextCursor::removeSelectedText()
{
    beginEditBlock();
    for (auto cursor = m_cursorList.begin(); cursor != m_cursorList.end(); ++cursor)
        cursor->removeSelectedText();
    endEditBlock();
    mergeCursors();
}

void MultiTextCursor::clearSelection()
{
    for (auto cursor = m_cursorList.begin(); cursor != m_cursorList.end(); ++cursor)
        cursor->clearSelection();
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
    if (m_cursorList.empty())
        return;

    m_cursorList.back().beginEditBlock();
    if (hasMultipleCursors()) {
        QStringList lines = text.split('\n');
        if (!lines.isEmpty() && lines.last().isEmpty())
            lines.pop_back();
        int index = 0;
        if (static_cast<long unsigned int>(lines.count()) == m_cursorList.size()) {
            for (const auto &element : std::as_const(m_cursorMap)) {
                QTextCursor &cursor = *element.second;
                insertAndSelect(cursor, lines.at(index++), selectNewText);
            }
            m_cursorList.back().endEditBlock();
            return;
        }
    }
    for (auto cursor = m_cursorList.begin(); cursor != m_cursorList.end(); ++cursor)
        insertAndSelect(*cursor, text, selectNewText);
    m_cursorList.back().endEditBlock();
}

bool equalCursors(const QTextCursor &lhs, const QTextCursor &rhs)
{
    return lhs == rhs && lhs.anchor() == rhs.anchor();
}

bool MultiTextCursor::operator==(const MultiTextCursor &other) const
{
    if (m_cursorList.size() != other.m_cursorList.size())
        return false;
    if (m_cursorList.empty())
        return true;

    if (!equalCursors(m_cursorList.back(), other.m_cursorList.back()))
        return false;

    auto it = m_cursorMap.begin();
    auto otherIt = other.m_cursorMap.begin();
    for (;it != m_cursorMap.end() && otherIt != other.m_cursorMap.end(); ++it, ++otherIt) {
        const QTextCursor &cursor = *it->second;
        const QTextCursor &otherCursor = *otherIt->second;
        if (it->first != otherIt->first || cursor != otherCursor
            || cursor.anchor() != otherCursor.anchor())
            return false;
    }
    return true;
}

bool MultiTextCursor::operator!=(const MultiTextCursor &other) const
{
    return !operator==(other);
}

void MultiTextCursor::mergeCursors()
{
    QList<QTextCursor> cursors(m_cursorList.begin(), m_cursorList.end());
    setCursors(cursors);
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

bool MultiTextCursor::multiCursorEvent(
    QKeyEvent *e, QKeySequence::StandardKey matchKey, Qt::KeyboardModifiers filterModifiers)
{
    filterModifiers |= (Utils::HostOsInfo::isMacHost() ? Qt::KeypadModifier : Qt::AltModifier);
    uint searchkey = (e->modifiers() | e->key()) & ~filterModifiers;

    const QList<QKeySequence> bindings = QKeySequence::keyBindings(matchKey);
    return bindings.contains(QKeySequence(searchkey));
}

bool MultiTextCursor::handleMoveKeyEvent(QKeyEvent *e, bool camelCaseNavigationEnabled)
{
    if (e->modifiers() & Qt::AltModifier && !Utils::HostOsInfo::isMacHost()) {
        QTextCursor::MoveOperation op = QTextCursor::NoMove;
        if (multiCursorEvent(e, QKeySequence::MoveToNextWord)) {
            op = QTextCursor::WordRight;
        } else if (multiCursorEvent(e, QKeySequence::MoveToPreviousWord)) {
            op = QTextCursor::WordLeft;
        } else if (multiCursorEvent(e, QKeySequence::MoveToEndOfBlock)) {
            op = QTextCursor::EndOfBlock;
        } else if (multiCursorEvent(e, QKeySequence::MoveToStartOfBlock)) {
            op = QTextCursor::StartOfBlock;
        } else if (multiCursorEvent(e, QKeySequence::MoveToNextLine, Qt::ShiftModifier)) {
            op = QTextCursor::Down;
        } else if (multiCursorEvent(e, QKeySequence::MoveToPreviousLine, Qt::ShiftModifier)) {
            op = QTextCursor::Up;
        } else if (multiCursorEvent(e, QKeySequence::MoveToStartOfLine)) {
            op = QTextCursor::StartOfLine;
        } else if (multiCursorEvent(e, QKeySequence::MoveToEndOfLine)) {
            op = QTextCursor::EndOfLine;
        } else if (multiCursorEvent(e, QKeySequence::MoveToStartOfDocument)) {
            op = QTextCursor::Start;
        } else if (multiCursorEvent(e, QKeySequence::MoveToEndOfDocument)) {
            op = QTextCursor::End;
        }

        if (op != QTextCursor::NoMove) {
            const std::list<QTextCursor> cursors = m_cursorList;
            for (QTextCursor cursor : cursors) {
                if (camelCaseNavigationEnabled && op == QTextCursor::WordRight)
                    CamelCaseCursor::right(&cursor, QTextCursor::MoveAnchor);
                else if (camelCaseNavigationEnabled && op == QTextCursor::WordLeft)
                    CamelCaseCursor::left(&cursor, QTextCursor::MoveAnchor);
                else
                    cursor.movePosition(op, QTextCursor::MoveAnchor);

                addCursor(cursor);
            }
            return true;
        }
    }

    for (auto it = m_cursorList.begin(); it != m_cursorList.end(); ++it) {
        QTextCursor &cursor = *it;
        QTextCursor::MoveMode mode = QTextCursor::MoveAnchor;
        QTextCursor::MoveOperation op = QTextCursor::NoMove;

        if (e == QKeySequence::MoveToNextChar) {
            op = QTextCursor::Right;
        } else if (e == QKeySequence::MoveToPreviousChar) {
            op = QTextCursor::Left;
        } else if (multiCursorEvent(e, QKeySequence::SelectNextChar)) {
            op = QTextCursor::Right;
            mode = QTextCursor::KeepAnchor;
        } else if (multiCursorEvent(e, QKeySequence::SelectPreviousChar)) {
            op = QTextCursor::Left;
            mode = QTextCursor::KeepAnchor;
        } else if (multiCursorEvent(e, QKeySequence::SelectNextWord)) {
            op = QTextCursor::WordRight;
            mode = QTextCursor::KeepAnchor;
        } else if (multiCursorEvent(e, QKeySequence::SelectPreviousWord)) {
            op = QTextCursor::WordLeft;
            mode = QTextCursor::KeepAnchor;
        } else if (multiCursorEvent(e, QKeySequence::SelectStartOfLine)) {
            op = QTextCursor::StartOfLine;
            mode = QTextCursor::KeepAnchor;
        } else if (multiCursorEvent(e, QKeySequence::SelectEndOfLine)) {
            op = QTextCursor::EndOfLine;
            mode = QTextCursor::KeepAnchor;
        } else if (multiCursorEvent(e, QKeySequence::SelectStartOfBlock)) {
            op = QTextCursor::StartOfBlock;
            mode = QTextCursor::KeepAnchor;
        } else if (multiCursorEvent(e, QKeySequence::SelectEndOfBlock)) {
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
            CamelCaseCursor::right(&cursor, mode);
        else if (camelCaseNavigationEnabled && op == QTextCursor::WordLeft)
            CamelCaseCursor::left(&cursor, mode);
        else if (!cursor.movePosition(op, mode) && mode == QTextCursor::MoveAnchor)
            cursor.clearSelection();
        cursor.setVisualNavigation(visualNavigation);
    }
    mergeCursors();
    return true;
}

} // namespace Utils
