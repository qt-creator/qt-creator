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

#include "qmljsautocompleter.h"

#include <qmljs/qmljsscanner.h>

#include <QChar>
#include <QLatin1Char>
#include <QTextDocument>
#include <QTextCursor>
#include <QTextBlock>

using namespace QmlJSEditor;
using namespace Internal;
using namespace QmlJS;

static int blockStartState(const QTextBlock &block)
{
    int state = block.previous().userState();

    if (state == -1)
        return 0;
    else
        return state & 0xff;
}

static Token tokenUnderCursor(const QTextCursor &cursor)
{
    const QString blockText = cursor.block().text();
    const int blockState = blockStartState(cursor.block());

    Scanner tokenize;
    const QList<Token> tokens = tokenize(blockText, blockState);
    const int pos = cursor.positionInBlock();

    int tokenIndex = 0;
    for (; tokenIndex < tokens.size(); ++tokenIndex) {
        const Token &token = tokens.at(tokenIndex);

        if (token.is(Token::Comment) || token.is(Token::String)) {
            if (pos > token.begin() && pos <= token.end())
                break;
        } else {
            if (pos >= token.begin() && pos < token.end())
                break;
        }
    }

    if (tokenIndex != tokens.size())
        return tokens.at(tokenIndex);

    return Token();
}

static bool shouldInsertMatchingText(QChar lookAhead)
{
    switch (lookAhead.unicode()) {
    case '{': case '}':
    case ']': case ')':
    case ';': case ',':
    case '"': case '\'':
        return true;

    default:
        if (lookAhead.isSpace())
            return true;

        return false;
    } // switch
}

static bool shouldInsertMatchingText(const QTextCursor &tc)
{
    QTextDocument *doc = tc.document();
    return shouldInsertMatchingText(doc->characterAt(tc.selectionEnd()));
}

static bool shouldInsertNewline(const QTextCursor &tc)
{
    QTextDocument *doc = tc.document();
    int pos = tc.selectionEnd();

    // count the number of empty lines.
    int newlines = 0;
    for (int e = doc->characterCount(); pos != e; ++pos) {
        const QChar ch = doc->characterAt(pos);

        if (! ch.isSpace())
            break;
        else if (ch == QChar::ParagraphSeparator)
            ++newlines;
    }

    if (newlines <= 1 && doc->characterAt(pos) != QLatin1Char('}'))
        return true;

    return false;
}

static bool isCompleteStringLiteral(const QStringRef &text)
{
    if (text.length() < 2)
        return false;

    const QChar quote = text.at(0);

    if (text.at(text.length() - 1) == quote)
        return text.at(text.length() - 2) != QLatin1Char('\\'); // ### not exactly.

    return false;
}

AutoCompleter::AutoCompleter()
{}

AutoCompleter::~AutoCompleter()
{}

bool AutoCompleter::contextAllowsAutoParentheses(const QTextCursor &cursor,
                                                 const QString &textToInsert) const
{
    QChar ch;

    if (! textToInsert.isEmpty())
        ch = textToInsert.at(0);

    switch (ch.unicode()) {
    case '\'':
    case '"':

    case '(':
    case '[':
    case '{':

    case ')':
    case ']':
    case '}':

    case ';':
        break;

    default:
        if (ch.isNull())
            break;

        return false;
    } // end of switch

    const Token token = tokenUnderCursor(cursor);
    switch (token.kind) {
    case Token::Comment:
        return false;

    case Token::String: {
        const QString blockText = cursor.block().text();
        const QStringRef tokenText = blockText.midRef(token.offset, token.length);
        QChar quote = tokenText.at(0);
        // if a string literal doesn't start with a quote, it must be multiline
        if (quote != QLatin1Char('"') && quote != QLatin1Char('\'')) {
            const int startState = blockStartState(cursor.block());
            if ((startState & Scanner::MultiLineMask) == Scanner::MultiLineStringDQuote)
                quote = QLatin1Char('"');
            else if ((startState & Scanner::MultiLineMask) == Scanner::MultiLineStringSQuote)
                quote = QLatin1Char('\'');
        }

        // never insert ' into string literals, it adds spurious ' when writing contractions
        if (ch == QLatin1Char('\''))
            return false;

        if (ch != quote || isCompleteStringLiteral(tokenText))
            break;

        return false;
    }

    default:
        break;
    } // end of switch

    return true;
}

bool AutoCompleter::contextAllowsElectricCharacters(const QTextCursor &cursor) const
{
    Token token = tokenUnderCursor(cursor);
    switch (token.kind) {
    case Token::Comment:
    case Token::String:
        return false;
    default:
        return true;
    }
}

bool AutoCompleter::isInComment(const QTextCursor &cursor) const
{
    return tokenUnderCursor(cursor).is(Token::Comment);
}

QString AutoCompleter::insertMatchingBrace(const QTextCursor &cursor,
                                           const QString &text,
                                           QChar,
                                           int *skippedChars) const
{
    if (text.length() != 1)
        return QString();

    if (! shouldInsertMatchingText(cursor))
        return QString();

    const QChar la = cursor.document()->characterAt(cursor.position());

    const QChar ch = text.at(0);
    switch (ch.unicode()) {
    case '\'':
        if (la != ch)
            return QString(ch);
        ++*skippedChars;
        break;

    case '"':
        if (la != ch)
            return QString(ch);
        ++*skippedChars;
        break;

    case '(':
        return QString(QLatin1Char(')'));

    case '[':
        return QString(QLatin1Char(']'));

    case '{':
        return QString(); // nothing to do.

    case ')':
    case ']':
    case '}':
    case ';':
        if (la == ch)
            ++*skippedChars;
        break;

    default:
        break;
    } // end of switch

    return QString();
}

QString AutoCompleter::insertParagraphSeparator(const QTextCursor &cursor) const
{
    if (shouldInsertNewline(cursor)) {
        QTextCursor selCursor = cursor;
        selCursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        if (! selCursor.selectedText().trimmed().isEmpty())
            return QString();

        return QLatin1String("}\n");
    }

    return QLatin1String("}");
}
