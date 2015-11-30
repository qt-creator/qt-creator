/****************************************************************************
**
** Copyright (C) 2015 Jan Dalheimer <jan@dalheimer.de>
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "cmakeautocompleter.h"

#include <QTextCursor>
#include <QTextBlock>
#include <QDebug>

#include <texteditor/tabsettings.h>

namespace CMakeProjectManager {
namespace Internal {

CMakeAutoCompleter::CMakeAutoCompleter()
{
    setAutoParenthesesEnabled(true);
}

bool CMakeAutoCompleter::isInComment(const QTextCursor &cursor) const
{
    // NOTE: This doesn't handle '#' inside quotes, nor multi-line comments
    QTextCursor moved = cursor;
    moved.movePosition(QTextCursor::StartOfLine, QTextCursor::KeepAnchor);
    if (moved.selectedText().contains(QLatin1Char('#')))
        return true;
    else
        return false;
}

bool CMakeAutoCompleter::isInString(const QTextCursor &cursor) const
{
    // NOTE: multiline strings are currently not supported, since they rarely, if ever, seem to be used
    QTextCursor moved = cursor;
    moved.movePosition(QTextCursor::StartOfLine);
    const int positionInLine = cursor.position() - moved.position();
    moved.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
    const QString line = moved.selectedText();

    bool isEscaped = false;
    bool inString = false;
    for (int i = 0; i < positionInLine; ++i) {
        const QChar c = line.at(i);
        if (c == QLatin1Char('\\') && !isEscaped)
            isEscaped = true;
        else if (c == QLatin1Char('"') && !isEscaped)
            inString = !inString;
        else
            isEscaped = false;
    }
    return inString;
}

QString CMakeAutoCompleter::insertMatchingBrace(const QTextCursor &cursor, const QString &text, QChar la, int *skippedChars) const
{
    Q_UNUSED(cursor)
    Q_UNUSED(skippedChars);
    if (text.isEmpty())
        return QString();
    const QChar current = text.at(0);
    switch (current.unicode()) {
    case '"':
        if (la != current)
            return QStringLiteral("\"");
        ++*skippedChars;
        break;

    case '(':
        return QStringLiteral(")");

    case ')':
        if (current == la)
            ++*skippedChars;
        break;

    default:
        break;
    }

    return QString();
}

int CMakeAutoCompleter::paragraphSeparatorAboutToBeInserted(QTextCursor &cursor, const TextEditor::TabSettings &tabSettings)
{
    const QString line = cursor.block().text().trimmed();
    if (line.contains(QRegExp(QStringLiteral("^(endfunction|endmacro|endif|endforeach|endwhile)\\w*\\("))))
        tabSettings.indentLine(cursor.block(), tabSettings.indentationColumn(cursor.block().text()));
    return 0;
}

bool CMakeAutoCompleter::contextAllowsAutoParentheses(const QTextCursor &cursor, const QString &textToInsert) const
{
    if (textToInsert.isEmpty())
        return false;

    const QChar c = textToInsert.at(0);
    if (c == QLatin1Char('"') || c == QLatin1Char('(') || c == QLatin1Char(')'))
        return !isInComment(cursor);
    return false;
}

bool CMakeAutoCompleter::contextAllowsElectricCharacters(const QTextCursor &cursor) const
{
    return !isInComment(cursor) && !isInString(cursor);
}

} // namespace Internal
} // namespace CMakeProjectManager
