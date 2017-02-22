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

#include "MatchingText.h"

#include "BackwardsScanner.h"

#include <cplusplus/Token.h>

#include <QTextDocument>
#include <QTextCursor>
#include <QChar>
#include <QDebug>

using namespace CPlusPlus;

enum { MAX_NUM_LINES = 20 };

static bool shouldOverrideChar(QChar ch)
{
    switch (ch.unicode()) {
    case ')': case ']': case ';': case '"': case '\'':
        return true;

    default:
        return false;
    }
}

static bool isCompleteStringLiteral(const BackwardsScanner &tk, int index)
{
    const QStringRef text = tk.textRef(index);

    if (text.length() < 2)
        return false;

    else if (text.at(text.length() - 1) == QLatin1Char('"'))
        return text.at(text.length() - 2) != QLatin1Char('\\'); // ### not exactly.

    return false;
}

static bool isEscaped(const QTextCursor &tc)
{
    const QTextDocument *doc = tc.document();

    int escapeCount = 0;
    int index = tc.selectionEnd() - 1;
    while (doc->characterAt(index) == '\\') {
        ++escapeCount;
        --index;
    }

    return (escapeCount % 2) != 0;
}

static bool isQuote(const QChar c)
{
    return c == '\'' || c == '"';
}

static bool insertQuote(const QChar ch, const BackwardsScanner &tk)
{
    // Always insert matching quote on an empty line
    if (tk.size() == 0)
        return true;

    const int index = tk.startToken() - 1;
    const Token &token = tk[index];

    // Insert an additional double quote after string literal only if previous literal was closed.
    if (ch == '"' && token.isStringLiteral() && isCompleteStringLiteral(tk, index))
        return true;

    // Insert a matching quote after an operator.
    if (token.isOperator())
        return true;

    if (token.isKeyword())
        return tk.text(index) == "operator";

    // Insert matching quote after identifier when identifier is a known literal prefixes
    static const QStringList stringLiteralPrefixes = {"L", "U", "u", "u8", "R"};
    return token.kind() == CPlusPlus::T_IDENTIFIER
            && stringLiteralPrefixes.contains(tk.text(index));
}

static int countSkippedChars(const QString blockText, const QString &textToProcess)
{
    int skippedChars = 0;
    const int length = qMin(blockText.length(), textToProcess.length());
    for (int i = 0; i < length; ++i) {
        const QChar ch1 = blockText.at(i);
        const QChar ch2 = textToProcess.at(i);

        if (ch1 != ch2)
            break;
        else if (! shouldOverrideChar(ch1))
            break;

        ++skippedChars;
    }
    return skippedChars;
}

static const Token tokenAtPosition(const Tokens &tokens, const unsigned pos)
{
    for (int i = tokens.size() - 1; i >= 0; --i) {
        const Token tk = tokens.at(i);
        if (pos >= tk.utf16charsBegin() && pos < tk.utf16charsEnd())
            return tk;
    }
    return Token();
}

bool MatchingText::contextAllowsAutoParentheses(const QTextCursor &cursor,
                                                const QString &textToInsert)
{
    QChar ch;

    if (!textToInsert.isEmpty())
        ch = textToInsert.at(0);

    if (!shouldInsertMatchingText(cursor) && ch != QLatin1Char('\'') && ch != QLatin1Char('"'))
        return false;
    else if (isInCommentHelper(cursor))
        return false;

    return true;
}

bool MatchingText::contextAllowsAutoQuotes(const QTextCursor &cursor, const QString &textToInsert)
{
    return !textToInsert.isEmpty() && !isInCommentHelper(cursor);
}

bool MatchingText::contextAllowsElectricCharacters(const QTextCursor &cursor)
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

bool MatchingText::shouldInsertMatchingText(const QTextCursor &cursor)
{
    QTextDocument *doc = cursor.document();
    return shouldInsertMatchingText(doc->characterAt(cursor.selectionEnd()));
}

bool MatchingText::shouldInsertMatchingText(QChar lookAhead)
{
    switch (lookAhead.unicode()) {
    case '{': case '}':
    case ']': case ')':
    case ';': case ',':
        return true;

    default:
        if (lookAhead.isSpace())
            return true;

        return false;
    } // switch
}

static Tokens getTokens(const QTextCursor &cursor, int &prevState)
{
    LanguageFeatures features;
    features.qtEnabled = false;
    features.qtKeywordsEnabled = false;
    features.qtMocRunEnabled = false;
    features.cxx11Enabled = true;
    features.cxxEnabled = true;
    features.c99Enabled = true;
    features.objCEnabled = true;

    SimpleLexer tokenize;
    tokenize.setLanguageFeatures(features);

    prevState = BackwardsScanner::previousBlockState(cursor.block()) & 0xFF;
    return tokenize(cursor.block().text(), prevState);
}

bool MatchingText::isInCommentHelper(const QTextCursor &cursor, Token *retToken)
{
    int prevState = 0;
    const Tokens tokens = getTokens(cursor, prevState);

    const unsigned pos = cursor.selectionEnd() - cursor.block().position();

    if (tokens.isEmpty() || pos < tokens.first().utf16charsBegin())
        return prevState > 0;

    if (pos >= tokens.last().utf16charsEnd()) {
        const Token tk = tokens.last();
        if (retToken)
            *retToken = tk;
        if (tk.is(T_CPP_COMMENT) || tk.is(T_CPP_DOXY_COMMENT))
            return true;
        return tk.isComment() && (cursor.block().userState() & 0xFF);
    }

    Token tk = tokenAtPosition(tokens, pos);
    if (retToken)
        *retToken = tk;
    return tk.isComment();
}

Kind MatchingText::stringKindAtCursor(const QTextCursor &cursor)
{
    int prevState = 0;
    const Tokens tokens = getTokens(cursor, prevState);

    const unsigned pos = cursor.selectionEnd() - cursor.block().position();

    if (tokens.isEmpty() || pos <= tokens.first().utf16charsBegin())
        return T_EOF_SYMBOL;

    if (pos >= tokens.last().utf16charsEnd()) {
        const Token tk = tokens.last();
        return tk.isStringLiteral() && prevState > 0 ? tk.kind() : T_EOF_SYMBOL;
    }

    Token tk = tokenAtPosition(tokens, pos);
    return tk.isStringLiteral() && pos > tk.utf16charsBegin() ? tk.kind() : T_EOF_SYMBOL;
}

QString MatchingText::insertMatchingBrace(const QTextCursor &cursor, const QString &textToProcess,
                                          QChar /*lookAhead*/, bool skipChars, int *skippedChars)
{
    if (textToProcess.isEmpty())
        return QString();

    QTextCursor tc = cursor;
    QString text = textToProcess;

    const QString blockText = tc.block().text().mid(tc.positionInBlock());
    const QString trimmedBlockText = blockText.trimmed();

    if (skipChars) {
        *skippedChars = countSkippedChars(blockText, textToProcess);
        if (*skippedChars != 0) {
            tc.movePosition(QTextCursor::NextCharacter, QTextCursor::MoveAnchor, *skippedChars);
            text = textToProcess.mid(*skippedChars);
        }
    }

    QString result;
    foreach (const QChar &ch, text) {
        if      (ch == QLatin1Char('('))  result += QLatin1Char(')');
        else if (ch == QLatin1Char('['))  result += QLatin1Char(']');
        // Handle '{' appearance within functinon call context
        else if (ch == QLatin1Char('{') && !trimmedBlockText.isEmpty() && trimmedBlockText.at(0) == QLatin1Char(')'))
            result += QLatin1Char('}');
    }

    return result;
}

QString MatchingText::insertMatchingQuote(const QTextCursor &cursor, const QString &textToProcess,
                                          QChar lookAhead, bool skipChars, int *skippedChars)
{
    if (textToProcess.isEmpty())
        return QString();

    QTextCursor tc = cursor;
    QString text = textToProcess;

    if (skipChars && !isEscaped(tc)) {
        const QString blockText = tc.block().text().mid(tc.positionInBlock());
        *skippedChars = countSkippedChars(blockText, textToProcess);
        if (*skippedChars != 0) {
            tc.movePosition(QTextCursor::NextCharacter, QTextCursor::MoveAnchor, *skippedChars);
            text = textToProcess.mid(*skippedChars);
        }
    }

    if (!shouldInsertMatchingText(lookAhead))
        return QString();

    if (!text.isEmpty()) {
        const QChar ch = text.at(0);
        if (!isQuote(ch))
            return QString();
        if (text.length() != 1)
            qWarning() << Q_FUNC_INFO << "handle event compression";

        BackwardsScanner tk(tc, LanguageFeatures::defaultFeatures(), MAX_NUM_LINES,
                            textToProcess.left(skippedChars ? *skippedChars : 0));
        if (insertQuote(ch, tk))
            return ch;
    }
    return QString();
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
        if (ch == QChar::ParagraphSeparator)
            ++newlines;
    }

    return newlines <= 1 && doc->characterAt(pos) != QLatin1Char('}');
}

QString MatchingText::insertParagraphSeparator(const QTextCursor &cursor)
{
    BackwardsScanner tk(cursor, LanguageFeatures::defaultFeatures(), MAX_NUM_LINES);
    int index = tk.startToken();

    if (tk[index - 1].isNot(T_LBRACE))
        return QString(); // nothing to do.

    const QString textBlock = cursor.block().text().mid(cursor.positionInBlock()).trimmed();
    if (! textBlock.isEmpty())
        return QString();

    --index; // consume the `{'

    const Token &token = tk[index - 1];

    if (token.is(T_IDENTIFIER)) {
        int i = index - 1;

        forever {
            const Token &current = tk[i - 1];

            if (current.is(T_EOF_SYMBOL))
                break;

            if (current.is(T_CLASS) || current.is(T_STRUCT) || current.is(T_UNION) || current.is(T_ENUM)) {
                // found a class key.
                QString str = QLatin1String("};");

                if (shouldInsertNewline(cursor))
                    str += QLatin1Char('\n');

                return str;
            }

            else if (current.is(T_NAMESPACE))
                return QLatin1String("}"); // found a namespace declaration

            else if (current.is(T_SEMICOLON))
                break; // found the `;' sync token

            else if (current.is(T_LBRACE) || current.is(T_RBRACE))
                break; // braces are considered sync tokens

            else if (current.is(T_LPAREN) || current.is(T_RPAREN))
                break; // sync token

            else if (current.is(T_LBRACKET) || current.is(T_RBRACKET))
                break; // sync token

            --i;
        }
    } else if (token.is(T_CLASS) || token.is(T_STRUCT) || token.is(T_UNION) || token.is(T_ENUM)) {
        if (tk[index - 2].is(T_TYPEDEF)) {
            // recognized:
            //   typedef struct {
            //
            // in this case we don't want to insert the extra semicolon+newline.
            return QLatin1String("}");
        }

        // anonymous class
        return QLatin1String("};");

    } else if (token.is(T_RPAREN)) {
        // search the matching brace.
        const int lparenIndex = tk.startOfMatchingBrace(index);

        if (lparenIndex == index) {
            // found an unmatched brace. We don't really know what to do in this case.
            return QString();
        }

        // look at the token before the matched brace
        const Token &tokenBeforeBrace = tk[lparenIndex - 1];

        if (tokenBeforeBrace.is(T_IF)) {
            // recognized an if statement
            return QLatin1String("}");

        } else if (tokenBeforeBrace.is(T_FOR) || tokenBeforeBrace.is(T_WHILE)) {
            // recognized a for-like statement
            return QLatin1String("}");

        }

        // if we reached this point there is a good chance that we are parsing a function definition
        QString str = QLatin1String("}");

        if (shouldInsertNewline(cursor))
            str += QLatin1Char('\n');

        return str;
    }

    // match the block
    return QLatin1String("}");
}
