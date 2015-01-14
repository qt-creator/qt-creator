/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
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

#include "cppautocompleter.h"

#include <cplusplus/MatchingText.h>
#include <cplusplus/BackwardsScanner.h>

#include <QLatin1Char>
#include <QTextCursor>

using namespace CppEditor;
using namespace Internal;
using namespace CPlusPlus;

static const Token tokenAtPosition(const Tokens &tokens, const unsigned pos)
{
    for (int i = tokens.size() - 1; i >= 0; --i) {
        const Token tk = tokens.at(i);
        if (pos >= tk.utf16charsBegin() && pos < tk.utf16charsEnd())
            return tk;
    }
    return Token();
}

static bool isInCommentHelper(const QTextCursor &cursor, Token *retToken = 0)
{
    LanguageFeatures features;
    features.qtEnabled = false;
    features.qtKeywordsEnabled = false;
    features.qtMocRunEnabled = false;
    features.cxx11Enabled = true;
    features.c99Enabled = true;

    SimpleLexer tokenize;
    tokenize.setLanguageFeatures(features);

    const int prevState = BackwardsScanner::previousBlockState(cursor.block()) & 0xFF;
    const Tokens tokens = tokenize(cursor.block().text(), prevState);

    const unsigned pos = cursor.selectionEnd() - cursor.block().position();

    if (tokens.isEmpty() || pos < tokens.first().utf16charsBegin())
        return prevState > 0;

    if (pos >= tokens.last().utf16charsEnd()) {
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

static bool isInStringHelper(const QTextCursor &cursor, Token *retToken = 0)
{
    LanguageFeatures features;
    features.qtEnabled = false;
    features.qtKeywordsEnabled = false;
    features.qtMocRunEnabled = false;
    features.cxx11Enabled = true;
    features.c99Enabled = true;

    SimpleLexer tokenize;
    tokenize.setLanguageFeatures(features);

    const int prevState = BackwardsScanner::previousBlockState(cursor.block()) & 0xFF;
    const Tokens tokens = tokenize(cursor.block().text(), prevState);

    const unsigned pos = cursor.selectionEnd() - cursor.block().position();

    if (tokens.isEmpty() || pos <= tokens.first().utf16charsBegin())
        return false;

    if (pos >= tokens.last().utf16charsEnd()) {
        const Token tk = tokens.last();
        return tk.isStringLiteral() && prevState > 0;
    }

    Token tk = tokenAtPosition(tokens, pos);
    if (retToken)
        *retToken = tk;
    return tk.isStringLiteral() && pos > tk.utf16charsBegin();
}

bool CppAutoCompleter::contextAllowsAutoParentheses(const QTextCursor &cursor,
                                                    const QString &textToInsert) const
{
    QChar ch;

    if (!textToInsert.isEmpty())
        ch = textToInsert.at(0);

    if (!(MatchingText::shouldInsertMatchingText(cursor)
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
        if (pos <= token.utf16charsEnd())
            return false;
    }

    return true;
}

bool CppAutoCompleter::isInComment(const QTextCursor &cursor) const
{
    return isInCommentHelper(cursor);
}

bool CppAutoCompleter::isInString(const QTextCursor &cursor) const
{
    return isInStringHelper(cursor);
}

QString CppAutoCompleter::insertMatchingBrace(const QTextCursor &cursor,
                                                const QString &text,
                                                QChar la,
                                                int *skippedChars) const
{
    return MatchingText::insertMatchingBrace(cursor, text, la, skippedChars);
}

QString CppAutoCompleter::insertParagraphSeparator(const QTextCursor &cursor) const
{
    return MatchingText::insertParagraphSeparator(cursor);
}

