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

/*
    This file is a self-contained interactive indenter for Qt Script.

    The general problem of indenting a program is ill posed. On
    the one hand, an indenter has to analyze programs written in a
    free-form formal language that is best described in terms of
    tokens, not characters, not lines. On the other hand, indentation
    applies to lines and white space characters matter, and otherwise
    the programs to indent are formally invalid in general, as they
    are begin edited.

    The approach taken here works line by line. We receive a program
    consisting of N lines or more, and we want to compute the
    indentation appropriate for the Nth line. Lines beyond the Nth
    lines are of no concern to us, so for simplicity we pretend the
    program has exactly N lines and we call the Nth line the "bottom
    line". Typically, we have to indent the bottom line when it's
    still empty, so we concentrate our analysis on the N - 1 lines
    that precede.

    By inspecting the (N - 1)-th line, the (N - 2)-th line, ...
    backwards, we determine the kind of the bottom line and indent it
    accordingly.

      * The bottom line is a comment line. See
        bottomLineStartsInCComment() and
        indentWhenBottomLineStartsInCComment().
      * The bottom line is a continuation line. See isContinuationLine()
        and indentForContinuationLine().
      * The bottom line is a standalone line. See
        indentForStandaloneLine().

    Certain tokens that influence the indentation, notably braces,
    are looked for in the lines. This is done by simple string
    comparison, without a real tokenizer. Confusing constructs such
    as comments and string literals are removed beforehand.
*/

#include <qmljs/qmljslineinfo.h>
#include <qmljs/qmljsscanner.h>

#include <QDebug>

using namespace QmlJS;

/*
    The indenter avoids getting stuck in almost infinite loops by
    imposing arbitrary limits on the number of lines it analyzes when
    looking for a construct.

    For example, the indenter never considers more than BigRoof lines
    backwards when looking for the start of a C-style comment.
*/
const int LineInfo::SmallRoof = 40;
const int LineInfo::BigRoof = 400;

LineInfo::LineInfo()
    : braceX(QRegExp(QLatin1String("^\\s*\\}\\s*(?:else|catch)\\b")))
{
    /*
        The "linizer" is a group of functions and variables to iterate
        through the source code of the program to indent. The program is
        given as a list of strings, with the bottom line being the line
        to indent. The actual program might contain extra lines, but
        those are uninteresting and not passed over to us.
    */

    // shorthands
    yyLine = 0;
    yyBraceDepth = 0;
    yyLeftBraceFollows = 0;
}

LineInfo::~LineInfo()
{
}

/*
    Returns the first non-space character in the string t, or
    QChar() if the string is made only of white space.
*/
QChar LineInfo::firstNonWhiteSpace(const QString &t) const
{
    int i = 0;
    while (i < t.length()) {
        if (!t.at(i).isSpace())
            return t.at(i);
        i++;
    }
    return QChar();
}

/*
   Removes some nefast constructs from a code line and returns the
   resulting line.
*/
QString LineInfo::trimmedCodeLine(const QString &t)
{
    Scanner scanner;

    QTextBlock currentLine = yyLinizerState.iter;
    int startState = qMax(0, currentLine.previous().userState()) & 0xff;

    yyLinizerState.tokens = scanner(t, startState);
    QString trimmed;
    int previousTokenEnd = 0;
    foreach (const Token &token, yyLinizerState.tokens) {
        trimmed.append(t.midRef(previousTokenEnd, token.begin() - previousTokenEnd));

        if (token.is(Token::String)) {
            for (int i = 0; i < token.length; ++i)
                trimmed.append(QLatin1Char('X'));

        } else if (token.is(Token::Comment)) {
            for (int i = 0; i < token.length; ++i)
                trimmed.append(QLatin1Char(' '));

        } else {
            trimmed.append(tokenText(token));
        }

        previousTokenEnd = token.end();
    }

    int index = yyLinizerState.tokens.size() - 1;
    for (; index != -1; --index) {
        const Token &token = yyLinizerState.tokens.at(index);
        if (token.isNot(Token::Comment))
            break;
    }

    bool isBinding = false;
    foreach (const Token &token, yyLinizerState.tokens) {
        if (token.is(Token::Colon)) {
            isBinding = true;
            break;
        }
    }

    if (index != -1) {
        const Token &last = yyLinizerState.tokens.at(index);
        bool needSemicolon = false;

        switch (last.kind) {
        case Token::LeftParenthesis:
        case Token::LeftBrace:
        case Token::LeftBracket:
        case Token::Semicolon:
        case Token::Delimiter:
            break;

        case Token::RightParenthesis:
        case Token::RightBrace:
        case Token::RightBracket:
            if (isBinding)
                needSemicolon = true;
            break;

        case Token::String:
        case Token::Number:
        case Token::Comma:
            needSemicolon = true;
            break;

        case Token::Identifier: {
            // need to disambiguate
            // "Rectangle\n{" in a QML context from
            // "a = Somevar\n{" in a JS context
            // What's done here does not cover all cases, but goes as far as possible
            // with the limited information that's available.
            const QStringRef text = tokenText(last);
            if (yyLinizerState.leftBraceFollows && !text.isEmpty() && text.at(0).isUpper()) {
                int i = index;

                // skip any preceding 'identifier.'; these could appear in both cases
                while (i >= 2) {
                    const Token &prev = yyLinizerState.tokens.at(i-1);
                    const Token &prevPrev = yyLinizerState.tokens.at(i-2);
                    if (prev.kind == Token::Dot && prevPrev.kind == Token::Identifier)
                        i -= 2;
                    else
                        break;
                }

                // it could also be 'a = \n Foo \n {', but that sounds unlikely
                if (i == 0)
                    break;

                // these indicate a QML context
                const Token &prev = yyLinizerState.tokens.at(i-1);
                if (prev.kind == Token::Semicolon || prev.kind == Token::Identifier
                        || prev.kind == Token::RightBrace || prev.kind == Token::RightBracket) {
                    break;
                }
            }
            needSemicolon = true;
            break;
        }

        case Token::Keyword:
            if (tokenText(last) != QLatin1String("else"))
                needSemicolon = true;
            break;

        default:
            break;
        } // end of switch

        if (needSemicolon) {
            const Token sc(trimmed.size(), 1, Token::Semicolon);
            yyLinizerState.tokens.append(sc);
            trimmed.append(QLatin1Char(';'));
            yyLinizerState.insertedSemicolon = true;
        }
    }

    return trimmed;
}

bool LineInfo::hasUnclosedParenOrBracket() const
{
    int closedParen = 0;
    int closedBracket = 0;
    for (int index = yyLinizerState.tokens.size() - 1; index != -1; --index) {
        const Token &token = yyLinizerState.tokens.at(index);

        if (token.is(Token::RightParenthesis)) {
            closedParen++;
        } else if (token.is(Token::RightBracket)) {
            closedBracket++;
        } else if (token.is(Token::LeftParenthesis)) {
            closedParen--;
            if (closedParen < 0)
                return true;
        } else if (token.is(Token::LeftBracket)) {
            closedBracket--;
            if (closedBracket < 0)
                return true;
        }
    }

    return false;
}

Token LineInfo::lastToken() const
{
    for (int index = yyLinizerState.tokens.size() - 1; index != -1; --index) {
        const Token &token = yyLinizerState.tokens.at(index);

        if (token.isNot(Token::Comment))
            return token;
    }

    return Token();
}

QStringRef LineInfo::tokenText(const Token &token) const
{
    return yyLinizerState.line.midRef(token.offset, token.length);
}

/*
    Saves and restores the state of the global linizer. This enables
    backtracking.
*/
#define YY_SAVE() LinizerState savedState = yyLinizerState
#define YY_RESTORE() yyLinizerState = savedState

/*
    Advances to the previous line in yyProgram and update yyLine
    accordingly. yyLine is cleaned from comments and other damageable
    constructs. Empty lines are skipped.
*/
bool LineInfo::readLine()
{
    int k;

    yyLinizerState.leftBraceFollows =
            (firstNonWhiteSpace(yyLinizerState.line) == QLatin1Char('{'));

    do {
        yyLinizerState.insertedSemicolon = false;

        if (yyLinizerState.iter == yyProgram.firstBlock()) {
            yyLinizerState.line.clear();
            return false;
        }

        yyLinizerState.iter = yyLinizerState.iter.previous();
        yyLinizerState.line = yyLinizerState.iter.text();

        yyLinizerState.line = trimmedCodeLine(yyLinizerState.line);

        /*
            Remove trailing spaces.
        */
        k = yyLinizerState.line.length();
        while (k > 0 && yyLinizerState.line.at(k - 1).isSpace())
            k--;
        yyLinizerState.line.truncate(k);

        /*
            '}' increment the brace depth and '{' decrements it and not
            the other way around, as we are parsing backwards.
        */
        yyLinizerState.braceDepth +=
                yyLinizerState.line.count(QLatin1Char('}')) + yyLinizerState.line.count(QLatin1Char(']')) -
                yyLinizerState.line.count(QLatin1Char('{')) - yyLinizerState.line.count(QLatin1Char('['));

        /*
            We use a dirty trick for

                } else ...

            We don't count the '}' yet, so that it's more or less
            equivalent to the friendly construct

                }
                else ...
        */
        if (yyLinizerState.pendingRightBrace)
            yyLinizerState.braceDepth++;
        yyLinizerState.pendingRightBrace =
                (yyLinizerState.line.indexOf(braceX) == 0);
        if (yyLinizerState.pendingRightBrace)
            yyLinizerState.braceDepth--;
    } while (yyLinizerState.line.isEmpty());

    return true;
}

/*
    Resets the linizer to its initial state, with yyLine containing the
    line above the bottom line of the program.
*/
void LineInfo::startLinizer()
{
    yyLinizerState.braceDepth = 0;
    yyLinizerState.pendingRightBrace = false;
    yyLinizerState.insertedSemicolon = false;

    yyLine = &yyLinizerState.line;
    yyBraceDepth = &yyLinizerState.braceDepth;
    yyLeftBraceFollows = &yyLinizerState.leftBraceFollows;

    yyLinizerState.iter = yyProgram.lastBlock();
    yyLinizerState.line = yyLinizerState.iter.text();
    readLine();
}

/*
    Returns true if the start of the bottom line of yyProgram (and
    potentially the whole line) is part of a C-style comment;
    otherwise returns false.
*/
bool LineInfo::bottomLineStartsInMultilineComment()
{
    QTextBlock currentLine = yyProgram.lastBlock().previous();
    QTextBlock previousLine = currentLine.previous();

    int startState = qMax(0, previousLine.userState()) & 0xff;
    if (startState > 0)
        return true;

    return false;
}

/*
    A function called match...() modifies the linizer state. If it
    returns true, yyLine is the top line of the matched construct;
    otherwise, the linizer is left in an unknown state.

    A function called is...() keeps the linizer state intact.
*/

/*
    Returns true if the current line (and upwards) forms a braceless
    control statement; otherwise returns false.

    The first line of the following example is a "braceless control
    statement":

        if (x)
            y;
*/
bool LineInfo::matchBracelessControlStatement()
{
    int delimDepth = 0;

    if (! yyLinizerState.tokens.isEmpty()) {
        Token tk = lastToken();

        if (tk.is(Token::Keyword) && tokenText(tk) == QLatin1String("else"))
            return true;

        else if (tk.isNot(Token::RightParenthesis))
            return false;
    }

    for (int i = 0; i < SmallRoof; i++) {
        for (int tokenIndex = yyLinizerState.tokens.size() - 1; tokenIndex != -1; --tokenIndex) {
            const Token &token = yyLinizerState.tokens.at(tokenIndex);

            switch (token.kind) {
            default:
                break;

            case Token::Comment:
                // skip comments
                break;

            case Token::RightParenthesis:
                ++delimDepth;
                break;

            case Token::LeftBrace:
            case Token::RightBrace:
            case Token::Semicolon:
                /*
                    We met a statement separator, but not where we
                    expected it. What follows is probably a weird
                    continuation line. Be careful with ';' in for,
                    though.
                */
                if (token.kind != Token::Semicolon || delimDepth == 0)
                    return false;
                break;


            case Token::LeftParenthesis:
                --delimDepth;

                if (delimDepth == 0 && tokenIndex > 0) {
                    const Token &tk = yyLinizerState.tokens.at(tokenIndex - 1);

                    if (tk.is(Token::Keyword)) {
                        const QStringRef text = tokenText(tk);

                        /*
                            We have

                                if-like (x)
                                    y

                            "if (x)" is not part of the statement
                            "y".
                        */


                        if      (tk.length == 5 && text == QLatin1String("catch"))
                            return true;

                        else if (tk.length == 2 && text == QLatin1String("do"))
                            return true;

                        else if (tk.length == 3 && text == QLatin1String("for"))
                            return true;

                        else if (tk.length == 2 && text == QLatin1String("if"))
                            return true;

                        else if (tk.length == 5 && text == QLatin1String("while"))
                            return true;

                        else if (tk.length == 4 && text == QLatin1String("with"))
                            return true;
                    }
                }

                if (delimDepth == -1) {
                    /*
                      We have

                          if ((1 +
                                2)

                      and not

                          if (1 +
                               2)
                    */
                    return false;
                }
                break;

            } // end of switch
        }

        if (! readLine())
            break;
    }

    return false;
}

/*
    Returns true if yyLine is an unfinished line; otherwise returns
    false.

    In many places we'll use the terms "standalone line", "unfinished
    line" and "continuation line". The meaning of these should be
    evident from this code example:

        a = b;    // standalone line
        c = d +   // unfinished line
            e +   // unfinished continuation line
            f +   // unfinished continuation line
            g;    // continuation line
*/
bool LineInfo::isUnfinishedLine()
{
    bool unf = false;

    YY_SAVE();

    if (yyLine->isEmpty())
        return false;

    const QChar lastCh = yyLine->at(yyLine->length() - 1);

    if (QString::fromLatin1("{};[]").indexOf(lastCh) == -1) {
        /*
          It doesn't end with ';' or similar. If it's not an "if (x)", it must be an unfinished line.
        */
        unf = ! matchBracelessControlStatement();

        if (unf && lastCh == QLatin1Char(')'))
            unf = false;

    } else if (lastCh == QLatin1Char(';')) {
        if (hasUnclosedParenOrBracket()) {
            /*
              Exception:

                  for (int i = 1; i < 10;
            */
            unf = true;

        // ### This only checks one line back.
        } else if (readLine() && yyLine->endsWith(QLatin1String(";")) && hasUnclosedParenOrBracket()) {
            /*
              Exception:

                  for (int i = 1;
                        i < 10;
            */
            unf = true;
        }
    }

    YY_RESTORE();
    return unf;
}

/*
    Returns true if yyLine is a continuation line; otherwise returns
    false.
*/
bool LineInfo::isContinuationLine()
{
    bool cont = false;

    YY_SAVE();
    if (readLine())
        cont = isUnfinishedLine();
    YY_RESTORE();
    return cont;
}


void LineInfo::initialize(QTextBlock begin, QTextBlock end)
{
    yyProgram = Program(begin, end);
    startLinizer();
}

