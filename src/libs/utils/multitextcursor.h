// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <QKeySequence>
#include <QTextCursor>

QT_BEGIN_NAMESPACE
class QKeyEvent;
class QPlainTextEdit;
QT_END_NAMESPACE

namespace Utils {

class QTCREATOR_UTILS_EXPORT MultiTextCursor
{
public:
    MultiTextCursor();
    explicit MultiTextCursor(const QList<QTextCursor> &cursors);

    MultiTextCursor(const MultiTextCursor &multiCursor);
    MultiTextCursor &operator=(const MultiTextCursor &multiCursor);
    MultiTextCursor(const MultiTextCursor &&multiCursor);
    MultiTextCursor &operator=(const MultiTextCursor &&multiCursor);

    ~MultiTextCursor();

    /// Replaces all cursors with \param cursors and the last one will be the new main cursors.
    void setCursors(const QList<QTextCursor> &cursors);
    const QList<QTextCursor> cursors() const;

    /// Returns whether this multi cursor contains any cursor.
    bool isNull() const;
    /// Returns whether this multi cursor contains more than one cursor.
    bool hasMultipleCursors() const;
    /// Returns the number of cursors handled by this cursor.
    int cursorCount() const;

    /// the \param cursor that is appended by added by \brief addCursor
    /// will be interpreted as the new main cursor
    void addCursor(const QTextCursor &cursor);
    void addCursors(const QList<QTextCursor> &cursors);
    /// convenience function that removes the old main cursor and appends
    /// \param cursor as the new main cursor
    void replaceMainCursor(const QTextCursor &cursor);
    /// Returns the main cursor.
    QTextCursor mainCursor() const;
    /// Returns the main cursor and removes it from this multi cursor.
    QTextCursor takeMainCursor();

    void beginEditBlock();
    void endEditBlock();
    /// merges overlapping cursors together
    void mergeCursors();

    /// applies the move key event \param e to all cursors in this multi cursor
    bool handleMoveKeyEvent(QKeyEvent *e, QPlainTextEdit *edit, bool camelCaseNavigationEnabled);
    /// applies the move \param operation to all cursors in this multi cursor \param n times
    /// with the move \param mode
    void movePosition(QTextCursor::MoveOperation operation, QTextCursor::MoveMode mode, int n = 1);

    /// Returns whether any cursor has a selection.
    bool hasSelection() const;
    /// Returns the selected text of all cursors that have a selection separated by
    /// a newline character.
    QString selectedText() const;
    /// removes the selected text of all cursors that have a selection from the document
    void removeSelectedText();

    /// inserts \param text into all cursors, potentially removing correctly selected text
    void insertText(const QString &text, bool selectNewText = false);

    bool operator==(const MultiTextCursor &other) const;
    bool operator!=(const MultiTextCursor &other) const;

    using iterator = std::list<QTextCursor>::iterator;
    using const_iterator = std::list<QTextCursor>::const_iterator;

    iterator begin() { return m_cursorList.begin(); }
    iterator end() { return m_cursorList.end(); }
    const_iterator begin() const { return m_cursorList.begin(); }
    const_iterator end() const { return m_cursorList.end(); }
    const_iterator constBegin() const { return m_cursorList.cbegin(); }
    const_iterator constEnd() const { return m_cursorList.cend(); }

    static bool multiCursorAddEvent(QKeyEvent *e, QKeySequence::StandardKey matchKey);

private:
    std::list<QTextCursor> m_cursorList;
    std::map<int, std::list<QTextCursor>::iterator> m_cursorMap;

    void fillMapWithList();
};

} // namespace Utils
