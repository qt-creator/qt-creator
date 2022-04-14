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

    /// replace all cursors with \param cursors and the last one will be the new main cursors
    void setCursors(const QList<QTextCursor> &cursors);
    const QList<QTextCursor> cursors() const;

    /// \returns whether this multi cursor contains any cursor
    bool isNull() const;
    /// \returns whether this multi cursor contains more than one cursor
    bool hasMultipleCursors() const;
    /// \returns the number of cursors handled by this cursor
    int cursorCount() const;

    /// the \param cursor that is appended by added by \brief addCursor
    /// will be interpreted as the new main cursor
    void addCursor(const QTextCursor &cursor);
    void addCursors(const QList<QTextCursor> &cursors);
    /// convenience function that removes the old main cursor and appends
    /// \param cursor as the new main cursor
    void replaceMainCursor(const QTextCursor &cursor);
    /// \returns the main cursor
    QTextCursor mainCursor() const;
    /// \returns the main cursor and removes it from this multi cursor
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

    /// \returns whether any cursor has a selection
    bool hasSelection() const;
    /// \returns the selected text of all cursors that have a selection separated by
    /// a newline character
    QString selectedText() const;
    /// removes the selected text of all cursors that have a selection from the document
    void removeSelectedText();

    /// inserts \param text into all cursors, potentially removing correctly selected text
    void insertText(const QString &text, bool selectNewText = false);

    bool operator==(const MultiTextCursor &other) const;
    bool operator!=(const MultiTextCursor &other) const;

    using iterator = QList<QTextCursor>::iterator;
    using const_iterator = QList<QTextCursor>::const_iterator;

    iterator begin() { return m_cursors.begin(); }
    iterator end() { return m_cursors.end(); }
    const_iterator begin() const { return m_cursors.begin(); }
    const_iterator end() const { return m_cursors.end(); }
    const_iterator constBegin() const { return m_cursors.constBegin(); }
    const_iterator constEnd() const { return m_cursors.constEnd(); }

    static bool multiCursorAddEvent(QKeyEvent *e, QKeySequence::StandardKey matchKey);

private:
    QList<QTextCursor> m_cursors;
};

} // namespace Utils
