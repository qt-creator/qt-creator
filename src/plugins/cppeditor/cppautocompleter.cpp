/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "cppautocompleter.h"

#include <Token.h>

#include <cplusplus/SimpleLexer.h>
#include <cplusplus/MatchingText.h>
#include <cplusplus/BackwardsScanner.h>

#include <QLatin1Char>
#include <QTextCursor>

using namespace CppEditor;
using namespace Internal;
using namespace CPlusPlus;

CppAutoCompleter::CppAutoCompleter()
{}

CppAutoCompleter::~CppAutoCompleter()
{}

bool CppAutoCompleter::contextAllowsAutoParentheses(const QTextCursor &cursor,
                                                    const QString &textToInsert) const
{
    QChar ch;

    if (! textToInsert.isEmpty())
        ch = textToInsert.at(0);

    if (! (MatchingText::shouldInsertMatchingText(cursor)
           || ch == QLatin1Char('\'')
           || ch == QLatin1Char('"')))
        return false;
    else if (isInCommentHelper(cursor))
        return false;

    return true;
}

bool CppAutoCompleter::contextAllowsElectricCharacters(const QTextCursor &cursor) const
{
    Token token;

    if (isInCommentHelper(cursor, &token))
        return false;

    if (token.isStringLiteral() || token.isCharLiteral()) {
        const unsigned pos = cursor.selectionEnd() - cursor.block().position();
        if (pos <= token.end())
            return false;
    }

    return true;
}

bool CppAutoCompleter::isInComment(const QTextCursor &cursor) const
{
    return isInCommentHelper(cursor);
}

QString CppAutoCompleter::insertMatchingBrace(const QTextCursor &cursor,
                                                const QString &text,
                                                QChar la,
                                                int *skippedChars) const
{
    MatchingText m;
    return m.insertMatchingBrace(cursor, text, la, skippedChars);
}

QString CppAutoCompleter::insertParagraphSeparator(const QTextCursor &cursor) const
{
    MatchingText m;
    return m.insertParagraphSeparator(cursor);
}

bool CppAutoCompleter::isInCommentHelper(const QTextCursor &cursor, Token *retToken) const
{
    SimpleLexer tokenize;
    tokenize.setQtMocRunEnabled(false);
    tokenize.setObjCEnabled(false);
    tokenize.setCxx0xEnabled(true);

    const int prevState = BackwardsScanner::previousBlockState(cursor.block()) & 0xFF;
    const QList<Token> tokens = tokenize(cursor.block().text(), prevState);

    const unsigned pos = cursor.selectionEnd() - cursor.block().position();

    if (tokens.isEmpty() || pos < tokens.first().begin())
        return prevState > 0;

    if (pos >= tokens.last().end()) {
        const Token tk = tokens.last();
        if (tk.is(T_CPP_COMMENT) || tk.is(T_CPP_DOXY_COMMENT))
            return true;
        return tk.isComment() && (cursor.block().userState() & 0xFF);
    }

    Token tk = tokenAtPosition(tokens, pos);

    if (retToken)
        *retToken = tk;

    return tk.isComment();
}

const Token CppAutoCompleter::tokenAtPosition(const QList<Token> &tokens, const unsigned pos) const
{
    for (int i = tokens.size() - 1; i >= 0; --i) {
        const Token tk = tokens.at(i);
        if (pos >= tk.begin() && pos < tk.end())
            return tk;
    }
    return Token();
}
