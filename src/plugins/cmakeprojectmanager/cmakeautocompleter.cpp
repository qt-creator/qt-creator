// Copyright (C) 2016 Jan Dalheimer <jan@dalheimer.de>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "cmakeautocompleter.h"

#include <QRegularExpression>

namespace CMakeProjectManager::Internal {

CMakeAutoCompleter::CMakeAutoCompleter()
{
    setAutoInsertBracketsEnabled(true);
}

bool CMakeAutoCompleter::isInComment(const QTextCursor &cursor) const
{
    // NOTE: This doesn't handle '#' inside quotes, nor multi-line comments
    QTextCursor moved = cursor;
    moved.movePosition(QTextCursor::StartOfLine, QTextCursor::KeepAnchor);
    return moved.selectedText().contains(QLatin1Char('#'));
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

QString CMakeAutoCompleter::insertMatchingBrace(const QTextCursor &cursor,
                                                const QString &text,
                                                QChar lookAhead,
                                                bool skipChars,
                                                int *skippedChars) const
{
    Q_UNUSED(cursor)
    if (text.isEmpty())
        return QString();
    const QChar current = text.at(0);
    switch (current.unicode()) {
    case '(':
        return QStringLiteral(")");

    case ')':
        if (current == lookAhead && skipChars)
            ++*skippedChars;
        break;

    default:
        break;
    }

    return QString();
}

QString CMakeAutoCompleter::insertMatchingQuote(const QTextCursor &cursor,
                                                const QString &text,
                                                QChar lookAhead,
                                                bool skipChars,
                                                int *skippedChars) const
{
    Q_UNUSED(cursor)
    static const QChar quote(QLatin1Char('"'));
    if (text.isEmpty() || text != quote)
        return QString();
    if (lookAhead == quote && skipChars) {
        ++*skippedChars;
        return QString();
    }
    return quote;
}

int CMakeAutoCompleter::paragraphSeparatorAboutToBeInserted(QTextCursor &cursor)
{
    const QString line = cursor.block().text().trimmed();
    if (line.contains(QRegularExpression(QStringLiteral("^(endfunction|endmacro|endif|endforeach|endwhile)\\w*\\("))))
        tabSettings().indentLine(cursor.block(), tabSettings().indentationColumn(cursor.block().text()));
    return 0;
}

bool CMakeAutoCompleter::contextAllowsAutoBrackets(const QTextCursor &cursor,
                                                   const QString &textToInsert) const
{
    if (textToInsert.isEmpty())
        return false;

    const QChar c = textToInsert.at(0);
    if (c == QLatin1Char('(') || c == QLatin1Char(')'))
        return !isInComment(cursor);
    return false;
}

bool CMakeAutoCompleter::contextAllowsAutoQuotes(const QTextCursor &cursor, const QString &textToInsert) const
{
    if (textToInsert.isEmpty())
        return false;

    const QChar c = textToInsert.at(0);
    if (c == QLatin1Char('"'))
        return !isInComment(cursor);
    return false;
}

bool CMakeAutoCompleter::contextAllowsElectricCharacters(const QTextCursor &cursor) const
{
    return !isInComment(cursor) && !isInString(cursor);
}

} // CMakeProjectManager::Internal
