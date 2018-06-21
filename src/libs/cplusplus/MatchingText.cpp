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

#include <utils/algorithm.h>
#include <utils/optional.h>

using namespace CPlusPlus;

enum { MAX_NUM_LINES = 20 };

static bool shouldOverrideChar(QChar ch)
{
    switch (ch.unicode()) {
    case ')': case ']': case '}': case ';': case '"': case '\'':
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
    if (token.isPunctuationOrOperator())
        return true;

    if (token.isKeyword())
        return tk.text(index) == "operator";

    // Insert matching quote after identifier when identifier is a known literal prefixes
    static const QStringList stringLiteralPrefixes = {"L", "U", "u", "u8", "R"};
    return token.kind() == CPlusPlus::T_IDENTIFIER
            && stringLiteralPrefixes.contains(tk.text(index));
}

static int countSkippedChars(const QString &blockText, const QString &textToProcess)
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

static LanguageFeatures languageFeatures()
{
    LanguageFeatures features;
    features.qtEnabled = false;
    features.qtKeywordsEnabled = false;
    features.qtMocRunEnabled = false;
    features.cxx11Enabled = true;
    features.cxxEnabled = true;
    features.c99Enabled = true;
    features.objCEnabled = true;

    return features;
}

static Tokens getTokens(const QTextCursor &cursor, int &prevState)
{
    SimpleLexer tokenize;
    tokenize.setLanguageFeatures(languageFeatures());

    prevState = BackwardsScanner::previousBlockState(cursor.block()) & 0xFF;
    return tokenize(cursor.block().text(), prevState);
}

static int isEmptyOrWhitespace(const QString &text)
{
    return Utils::allOf(text, [](const QChar &c) {return c.isSpace(); });
}

static bool isCursorAtEndOfLineButMaybeBeforeComment(const QTextCursor &cursor)
{
    const QString textAfterCursor = cursor.block().text().mid(cursor.positionInBlock());
    if (isEmptyOrWhitespace(textAfterCursor))
        return true;

    SimpleLexer tokenize;
    tokenize.setLanguageFeatures(languageFeatures());
    const Tokens tokens = tokenize(textAfterCursor);
    return Utils::allOf(tokens, [](const Token &token) { return token.isComment(); });
}

using TokenIndexResult = Utils::optional<int>;

// 10.6.1 Attribute syntax and semantics
// This does not handle alignas() since it is not needed for the namespace case.
static TokenIndexResult skipAttributeSpecifierSequence(const BackwardsScanner &tokens, int index)
{
    if (tokens[index].is(T_EOF_SYMBOL))
        return {};

    // [[ attribute-using-prefixopt attribute-list ]]
    if (tokens[index].is(T_RBRACKET) && tokens[index - 1].is(T_RBRACKET)) {
        // Skip everything within [[ ]]
        for (int i = index - 2; !tokens[i].is(T_EOF_SYMBOL); --i) {
            if (tokens[i].is(T_LBRACKET) && tokens[i - 1].is(T_LBRACKET))
                return i - 2;
        }

        return {};
    }

    return index;
}

static TokenIndexResult skipNamespaceName(const BackwardsScanner &tokens, int index)
{
    if (tokens[index].is(T_EOF_SYMBOL))
        return {};

    if (!tokens[index].is(T_IDENTIFIER))
        return index;

    // Accept
    //  SomeName
    //  Some::Nested::Name
    bool expectIdentifier = false;
    for (int i = index - 1; !tokens[i].is(T_EOF_SYMBOL); --i) {
        if (expectIdentifier) {
            if (tokens[i].is(T_IDENTIFIER))
                expectIdentifier = false;
            else
                return {};
        } else if (tokens[i].is(T_COLON_COLON)) {
            expectIdentifier = true;
        } else {
            return i;
        }
    }

    return index;
}

// 10.3.1 Namespace definition
static bool isAfterNamespaceDefinition(const BackwardsScanner &tokens, int index)
{
    if (tokens[index].is(T_EOF_SYMBOL))
        return false;

    // Handle optional name
    TokenIndexResult newIndex = skipNamespaceName(tokens, index);
    if (!newIndex)
        return false;

    // Handle optional attribute specifier sequence
    newIndex = skipAttributeSpecifierSequence(tokens, newIndex.value());
    if (!newIndex)
        return false;

    return tokens[newIndex.value()].is(T_NAMESPACE);
}

static QTextBlock previousNonEmptyBlock(const QTextBlock &currentBlock)
{
    QTextBlock block = currentBlock.previous();
    forever {
        if (!block.isValid() || !isEmptyOrWhitespace(block.text()))
            return block;
        block = block.previous();
    }
}

static QTextBlock nextNonEmptyBlock(const QTextBlock &currentBlock)
{
    QTextBlock block = currentBlock.next();
    forever {
        if (!block.isValid() || !isEmptyOrWhitespace(block.text()))
            return block;
        block = block.next();
    }
}

static bool allowAutoClosingBraceAtEmptyLine(
        const QTextBlock &block,
        MatchingText::IsNextBlockDeeperIndented isNextDeeperIndented)
{
    QTextBlock previousBlock = previousNonEmptyBlock(block);
    if (!previousBlock.isValid())
        return false; // Nothing before

    QTextBlock nextBlock = nextNonEmptyBlock(block);
    if (!nextBlock.isValid())
        return true; // Nothing behind

    if (isNextDeeperIndented && isNextDeeperIndented(previousBlock))
        return false; // Before indented

    const QString trimmedText = previousBlock.text().trimmed();
    return !trimmedText.endsWith(';')
        && !trimmedText.endsWith('{')
        && !trimmedText.endsWith('}');
}

static QChar firstNonSpace(const QTextCursor &cursor)
{
    int position = cursor.position();
    QChar ch = cursor.document()->characterAt(position);
    while (ch.isSpace())
        ch = cursor.document()->characterAt(++position);

    return ch;
}

static bool allowAutoClosingBraceByLookahead(const QTextCursor &cursor)
{
    const QChar lookAhead = firstNonSpace(cursor);
    if (lookAhead.isNull())
        return true;

    switch (lookAhead.unicode()) {
    case ';': case ',':
    case ')': case '}': case ']':
        return true;
    }

    return false;
}

static bool isRecordLikeToken(const Token &token)
{
    return token.is(T_CLASS)
        || token.is(T_STRUCT)
        || token.is(T_UNION)
        || token.is(T_ENUM);
}

static bool isRecordLikeToken(const BackwardsScanner &tokens, int index)
{
    if (index + tokens.offset() < tokens.size() - 1)
        return isRecordLikeToken(tokens[index]);
    return false;
}

static bool recordLikeHasToFollowToken(const Token &token)
{
    return token.is(T_SEMICOLON)
        || token.is(T_LBRACE) // e.g. class X {
        || token.is(T_RBRACE) // e.g. function definition
        || token.is(T_EOF_SYMBOL);
}

static bool recordLikeMightFollowToken(const Token &token)
{
    return token.is(T_IDENTIFIER) // e.g. macro like QT_END_NAMESPACE
        || token.is(T_ANGLE_STRING_LITERAL) // e.g. #include directive
        || token.is(T_STRING_LITERAL) // e.g. #include directive
        || token.isComment();
}

static bool isAfterRecordLikeDefinition(const BackwardsScanner &tokens, int index)
{
    for (;; --index) {
        if (recordLikeHasToFollowToken(tokens[index]))
            return isRecordLikeToken(tokens, index + 1);

        if (recordLikeMightFollowToken(tokens[index]) && isRecordLikeToken(tokens, index + 1))
            return true;
    }

    return false;
}

static bool allowAutoClosingBrace(const QTextCursor &cursor,
                                  MatchingText::IsNextBlockDeeperIndented isNextIndented)
{
    if (MatchingText::isInCommentHelper(cursor))
        return false;

    BackwardsScanner tokens(cursor, languageFeatures(), /*maxBlockCount=*/ 5);
    const int index = tokens.startToken() - 1;

    if (tokens[index].isStringLiteral())
        return false;

    if (isAfterNamespaceDefinition(tokens, index))
        return false;

    if (isAfterRecordLikeDefinition(tokens, index))
        return false;

    const QTextBlock block = cursor.block();
    if (isEmptyOrWhitespace(block.text()))
        return allowAutoClosingBraceAtEmptyLine(cursor.block(), isNextIndented);

    if (isCursorAtEndOfLineButMaybeBeforeComment(cursor))
        return !(isNextIndented && isNextIndented(block));

    return allowAutoClosingBraceByLookahead(cursor);
}

bool MatchingText::contextAllowsAutoParentheses(const QTextCursor &cursor,
                                                const QString &textToInsert,
                                                IsNextBlockDeeperIndented isNextIndented)
{
    QChar ch;

    if (!textToInsert.isEmpty())
        ch = textToInsert.at(0);

    if (ch == QLatin1Char('{'))
        return allowAutoClosingBrace(cursor, isNextIndented);

    if (!shouldInsertMatchingText(cursor) && ch != QLatin1Char('\'') && ch != QLatin1Char('"'))
        return false;

    if (isInCommentHelper(cursor))
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

    QString text = textToProcess;

    if (skipChars) {
        QTextCursor tc = cursor;
        const QString blockText = tc.block().text().mid(tc.positionInBlock());
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
        else if (ch == QLatin1Char('{'))  result += QLatin1Char('}');
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

            if (isRecordLikeToken(current)) {
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
