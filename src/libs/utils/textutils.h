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

#pragma once

#include "linecolumn.h"
#include "utils_global.h"

#include <QString>
#include <QTextCursor>

QT_FORWARD_DECLARE_CLASS(QTextDocument)
QT_FORWARD_DECLARE_CLASS(QTextCursor)

namespace Utils {
namespace Text {

// line is 1-based, column is 0-based
QTCREATOR_UTILS_EXPORT bool convertPosition(const QTextDocument *document,
                                            int pos,
                                            int *line, int *column);
QTCREATOR_UTILS_EXPORT
OptionalLineColumn convertPosition(const QTextDocument *document, int pos);

// line and column are 1-based
QTCREATOR_UTILS_EXPORT int positionInText(QTextDocument *textDocument, int line, int column);

QTCREATOR_UTILS_EXPORT QString textAt(QTextCursor tc, int pos, int length);

QTCREATOR_UTILS_EXPORT QTextCursor selectAt(QTextCursor textCursor, uint line, uint column, uint length);

QTCREATOR_UTILS_EXPORT QTextCursor flippedCursor(const QTextCursor &cursor);

QTCREATOR_UTILS_EXPORT QTextCursor wordStartCursor(const QTextCursor &cursor);

template <class CharacterProvider>
void moveToPrevChar(CharacterProvider &provider, QTextCursor &cursor)
{
    cursor.movePosition(QTextCursor::PreviousCharacter);
    while (provider.characterAt(cursor.position()).isSpace())
        cursor.movePosition(QTextCursor::PreviousCharacter);
}

template <class CharacterProvider>
bool matchPreviousWord(CharacterProvider &provider, QTextCursor cursor, QString pattern)
{
    cursor.movePosition(QTextCursor::PreviousWord);
    while (provider.characterAt(cursor.position()) == ':')
        cursor.movePosition(QTextCursor::PreviousWord, QTextCursor::MoveAnchor, 2);

    int previousWordStart = cursor.position();
    cursor.movePosition(QTextCursor::NextWord);
    moveToPrevChar(provider, cursor);
    QString toMatch = provider.textAt(previousWordStart, cursor.position() - previousWordStart + 1);

    pattern = pattern.simplified();
    while (!pattern.isEmpty() && pattern.endsWith(toMatch)) {
        pattern.chop(toMatch.length());
        if (pattern.endsWith(' '))
            pattern.chop(1);
        if (!pattern.isEmpty()) {
            cursor.movePosition(QTextCursor::StartOfWord);
            cursor.movePosition(QTextCursor::PreviousWord);
            previousWordStart = cursor.position();
            cursor.movePosition(QTextCursor::NextWord);
            moveToPrevChar(provider, cursor);
            toMatch = provider.textAt(previousWordStart, cursor.position() - previousWordStart + 1);
        }
    }
    return pattern.isEmpty();
}

} // Text
} // Utils
