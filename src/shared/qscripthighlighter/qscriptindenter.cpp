/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

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

#include "qscriptindenter.h"
#include "qscriptincrementalscanner.h"
#include <QtDebug>

using namespace SharedTools;

/*
    The indenter avoids getting stuck in almost infinite loops by
    imposing arbitrary limits on the number of lines it analyzes when
    looking for a construct.

    For example, the indenter never considers more than BigRoof lines
    backwards when looking for the start of a C-style comment.
*/
const int QScriptIndenter::SmallRoof = 40;
const int QScriptIndenter::BigRoof = 400;

QScriptIndenter::QScriptIndenter()
    : label(QRegExp(QLatin1String("^\\s*((?:case\\b([^:])+|[a-zA-Z_0-9.]+)(?:\\s+)?:)(?!:)"))),
      braceX(QRegExp(QLatin1String("^\\s*\\}\\s*(?:else|catch)\\b"))),
      iflikeKeyword(QRegExp(QLatin1String("\\b(?:catch|do|for|if|while|with)\\b")))
{

    /*
        The indenter supports a few parameters:

          * ppHardwareTabSize is the size of a '\t' in your favorite editor.
          * ppIndentSize is the size of an indentation, or software tab
            size.
          * ppContinuationIndentSize is the extra indent for a continuation
            line, when there is nothing to align against on the previous
            line.
          * ppCommentOffset is the indentation within a C-style comment,
            when it cannot be picked up.
    */

    ppHardwareTabSize = 8;
    ppIndentSize = 4;
    ppContinuationIndentSize = 8;
    ppCommentOffset = 2;

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

QScriptIndenter::~QScriptIndenter()
{
}

void QScriptIndenter::setTabSize(int size)
{
    ppHardwareTabSize = size;
}

void QScriptIndenter::setIndentSize(int size)
{
    ppIndentSize = size;
    ppContinuationIndentSize = 2 * size;
}

/*
    Returns the first non-space character in the string t, or
    QChar() if the string is made only of white space.
*/
QChar QScriptIndenter::firstNonWhiteSpace(const QString &t) const
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
    Returns true if string t is made only of white space; otherwise
    returns false.
*/
bool QScriptIndenter::isOnlyWhiteSpace(const QString &t) const
{
    return firstNonWhiteSpace(t).isNull();
}

/*
    Assuming string t is a line, returns the column number of a given
    index. Column numbers and index are identical for strings that don't
    contain '\t's.
*/
int QScriptIndenter::columnForIndex(const QString &t, int index) const
{
    int col = 0;
    if (index > t.length())
        index = t.length();

    for (int i = 0; i < index; i++) {
        if (t.at(i) == QLatin1Char('\t')) {
            col = ((col / ppHardwareTabSize) + 1) * ppHardwareTabSize;
        } else {
            col++;
        }
    }
    return col;
}

/*
    Returns the indentation size of string t.
*/
int QScriptIndenter::indentOfLine(const QString &t) const
{
    return columnForIndex(t, t.indexOf(firstNonWhiteSpace(t)));
}

/*
    Replaces t[k] by ch, unless t[k] is '\t'. Tab characters are better
    left alone since they break the "index equals column" rule. No
    provisions are taken against '\n' or '\r', which shouldn't occur in
    t anyway.
*/
void QScriptIndenter::eraseChar(QString &t, int k, QChar ch) const
{
    if (t.at(k) != QLatin1Char('\t'))
        t[k] = ch;
}

/*
   Removes some nefast constructs from a code line and returns the
   resulting line.
*/
QString QScriptIndenter::trimmedCodeLine(const QString &t)
{
    QScriptIncrementalScanner scanner;

    QTextBlock currentLine = yyLinizerState.iter;
    int startState = qMax(0, currentLine.previous().userState()) & 0xff;

    yyLinizerState.tokens = scanner(t, startState);
    QString trimmed;
    int previousTokenEnd = 0;
    foreach (const QScriptIncrementalScanner::Token &token, yyLinizerState.tokens) {
        trimmed.append(t.midRef(previousTokenEnd, token.begin() - previousTokenEnd));

        if (token.is(QScriptIncrementalScanner::Token::String)) {
            for (int i = 0; i < token.length; ++i)
                trimmed.append(QLatin1Char('X'));

        } else if (token.is(QScriptIncrementalScanner::Token::Comment)) {
            for (int i = 0; i < token.length; ++i)
                trimmed.append(QLatin1Char(' '));

        } else {
            trimmed.append(t.midRef(token.offset, token.length));
        }

        previousTokenEnd = token.end();
    }

    int index = yyLinizerState.tokens.size() - 1;
    for (; index != -1; --index) {
        const QScriptIncrementalScanner::Token &token = yyLinizerState.tokens.at(index);
        if (token.isNot(QScriptIncrementalScanner::Token::Comment))
            break;
    }

    bool isBinding = false;
    foreach (const QScriptIncrementalScanner::Token &token, yyLinizerState.tokens) {
        if (token.is(QScriptIncrementalScanner::Token::Colon)) {
            isBinding = true;
            break;
        }
    }

    if (index != -1) {
        const QScriptIncrementalScanner::Token &last = yyLinizerState.tokens.at(index);

        switch (last.kind) {
        case QScriptIncrementalScanner::Token::LeftParenthesis:
        case QScriptIncrementalScanner::Token::LeftBrace:
        case QScriptIncrementalScanner::Token::Semicolon:
        case QScriptIncrementalScanner::Token::Operator:
            break;

        case QScriptIncrementalScanner::Token::RightParenthesis:
        case QScriptIncrementalScanner::Token::RightBrace:
            if (isBinding)
                trimmed.append(QLatin1Char(';'));
            break;

        case QScriptIncrementalScanner::Token::Colon:
        case QScriptIncrementalScanner::Token::LeftBracket:
        case QScriptIncrementalScanner::Token::RightBracket:
            trimmed.append(QLatin1Char(';'));
            break;

        case QScriptIncrementalScanner::Token::Identifier:
        case QScriptIncrementalScanner::Token::Keyword:
            if (t.midRef(last.offset, last.length) != QLatin1String("else"))
                trimmed.append(QLatin1Char(';'));
            break;

        default:
            trimmed.append(QLatin1Char(';'));
            break;
        } // end of switch
    }

    return trimmed;
}

/*
    Returns '(' if the last parenthesis is opening, ')' if it is
    closing, and QChar() if there are no parentheses in t.
*/
QChar QScriptIndenter::lastParen(const QString &t) const
{
    int i = t.length();
    while (i > 0) {
        i--;
        if (t.at(i) == QLatin1Char('(') || t.at(i) == QLatin1Char(')'))
            return t.at(i);
    }
    return QChar();
}

/*
    Returns true if typedIn the same as okayCh or is null; otherwise
    returns false.
*/
bool QScriptIndenter::okay(QChar typedIn, QChar okayCh) const
{
    return typedIn == QChar() || typedIn == okayCh;
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
bool QScriptIndenter::readLine()
{
    int k;

    yyLinizerState.leftBraceFollows =
            (firstNonWhiteSpace(yyLinizerState.line) == QLatin1Char('{'));

    do {
        if (yyLinizerState.iter == yyProgram.firstBlock()) {
            yyLinizerState.line.clear();
            return false;
        }

        yyLinizerState.iter = yyLinizerState.iter.previous();
        yyLinizerState.line = yyLinizerState.iter.text();

        yyLinizerState.line = trimmedCodeLine(yyLinizerState.line);

        /*
            Remove preprocessor directives.
        */
        k = 0;
        while (k < yyLinizerState.line.length()) {
            const QChar ch = yyLinizerState.line.at(k);
            if (ch == QLatin1Char('#')) {
                yyLinizerState.line.clear();
            } else if (!ch.isSpace()) {
                break;
            }
            k++;
        }

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
                yyLinizerState.line.count('}') -
                yyLinizerState.line.count('{');

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
void QScriptIndenter::startLinizer()
{
    yyLinizerState.braceDepth = 0;
    yyLinizerState.pendingRightBrace = false;

    yyLine = &yyLinizerState.line;
    yyBraceDepth = &yyLinizerState.braceDepth;
    yyLeftBraceFollows = &yyLinizerState.leftBraceFollows;

    yyLinizerState.iter = yyProgram.lastBlock();
    yyLinizerState.iter = yyLinizerState.iter.previous();
    yyLinizerState.line = yyLinizerState.iter.text();
    readLine();
}

/*
    Returns true if the start of the bottom line of yyProgram (and
    potentially the whole line) is part of a C-style comment;
    otherwise returns false.
*/
bool QScriptIndenter::bottomLineStartsInMultilineComment()
{
    QTextBlock currentLine = yyProgram.lastBlock().previous();
    QTextBlock previousLine = currentLine.previous();

    int startState = qMax(0, previousLine.userState()) & 0xff;
    if (startState > 0)
        return true;

    return false;
}

/*
    Returns the recommended indent for the bottom line of yyProgram
    assuming that it starts in a C-style comment, a condition that is
    tested elsewhere.

    Essentially, we're trying to align against some text on the
    previous line.
*/
int QScriptIndenter::indentWhenBottomLineStartsInMultiLineComment()
{
    QTextBlock block = yyProgram.lastBlock().previous();
    QString blockText;

    for (; block.isValid(); block = block.previous()) {
        blockText = block.text();

        if (! isOnlyWhiteSpace(blockText))
            break;
    }

    return indentOfLine(blockText);
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
bool QScriptIndenter::matchBracelessControlStatement()
{
    int delimDepth = 0;

    if (yyLine->endsWith(QLatin1String("else")))
        return true;

    if (!yyLine->endsWith(QLatin1String(")")))
        return false;

    for (int i = 0; i < SmallRoof; i++) {
        int j = yyLine->length();
        while (j > 0) {
            j--;
            QChar ch = yyLine->at(j);

            switch (ch.unicode()) {
            case ')':
                delimDepth++;
                break;
            case '(':
                delimDepth--;
                if (delimDepth == 0) {
                    if (yyLine->indexOf(iflikeKeyword) != -1) {
                        /*
                            We have

                                if (x)
                                    y

                            "if (x)" is not part of the statement
                            "y".
                        */
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
            case '{':
            case '}':
            case ';':
                /*
                    We met a statement separator, but not where we
                    expected it. What follows is probably a weird
                    continuation line. Be careful with ';' in for,
                    though.
                */
                if (ch != QLatin1Char(';') || delimDepth == 0)
                    return false;
            }
        }

        if (!readLine())
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
bool QScriptIndenter::isUnfinishedLine()
{
    bool unf = false;

    YY_SAVE();

    if (yyLine->isEmpty())
        return false;

    QChar lastCh = yyLine->at(yyLine->length() - 1);
    if (QString::fromLatin1("{};").indexOf(lastCh) == -1) {
        /*
          It doesn't end with ';' or similar. If it's not an "if (x)", it must be an unfinished line.
        */
        unf = ! matchBracelessControlStatement();

        if (unf && lastCh == QLatin1Char(')'))
            unf = false;

    } else if (lastCh == QLatin1Char(';')) {
        if (lastParen(*yyLine) == QLatin1Char('(')) {
            /*
              Exception:

                  for (int i = 1; i < 10;
            */
            unf = true;
        } else if (readLine() && yyLine->endsWith(QLatin1String(";")) &&
                    lastParen(*yyLine) == QLatin1Char('(')) {
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
bool QScriptIndenter::isContinuationLine()
{
    bool cont = false;

    YY_SAVE();
    if (readLine())
        cont = isUnfinishedLine();
    YY_RESTORE();
    return cont;
}

/*
    Returns the recommended indent for the bottom line of yyProgram,
    assuming it's a continuation line.

    We're trying to align the continuation line against some parenthesis
    or other bracked left opened on a previous line, or some interesting
    operator such as '='.
*/
int QScriptIndenter::indentForContinuationLine()
{
    int braceDepth = 0;
    int delimDepth = 0;

    bool leftBraceFollowed = *yyLeftBraceFollows;

    for (int i = 0; i < SmallRoof; i++) {
        int hook = -1;

        int j = yyLine->length();
        while (j > 0 && hook < 0) {
            j--;
            QChar ch = yyLine->at(j);

            switch (ch.unicode()) {
            case ')':
                delimDepth++;
                break;
            case ']':
                braceDepth++;
                break;
            case '}':
                braceDepth++;
                break;
            case '(':
                delimDepth--;
                /*
                    An unclosed delimiter is a good place to align at,
                    at least for some styles (including Qt's).
                */
                if (delimDepth == -1)
                    hook = j;
                break;

            case '[':
                braceDepth--;
                /*
                    An unclosed delimiter is a good place to align at,
                    at least for some styles (including Qt's).
                */
                if (braceDepth == -1)
                    hook = j;
                break;
            case '{':
                braceDepth--;
                /*
                    A left brace followed by other stuff on the same
                    line is typically for an enum or an initializer.
                    Such a brace must be treated just like the other
                    delimiters.
                */
                if (braceDepth == -1) {
                    if (j < yyLine->length() - 1) {
                        hook = j;
                    } else {
                        return 0; // shouldn't happen
                    }
                }
                break;
            case '=':
                /*
                    An equal sign is a very natural alignment hook
                    because it's usually the operator with the lowest
                    precedence in statements it appears in. Case in
                    point:

                        int x = 1 +
                                2;

                    However, we have to beware of constructs such as
                    default arguments and explicit enum constant
                    values:

                        void foo(int x = 0,
                                  int y = 0);

                    And not

                        void foo(int x = 0,
                                        int y = 0);

                    These constructs are caracterized by a ',' at the
                    end of the unfinished lines or by unbalanced
                    parentheses.
                */
                Q_ASSERT(j - 1 >= 0);

                if (QString::fromLatin1("!=<>").indexOf(yyLine->at(j - 1)) == -1 &&
                     j + 1 < yyLine->length() && yyLine->at(j + 1) != '=') {
                    if (braceDepth == 0 && delimDepth == 0 &&
                         j < yyLine->length() - 1 &&
                         !yyLine->endsWith(QLatin1String(",")) &&
                         (yyLine->contains('(') == yyLine->contains(')')))
                        hook = j;
                }
            }
        }

        if (hook >= 0) {
            /*
                Yes, we have a delimiter or an operator to align
                against! We don't really align against it, but rather
                against the following token, if any. In this example,
                the following token is "11":

                    int x = (11 +
                              2);

                If there is no such token, we use a continuation indent:

                    static QRegExp foo(QString(
                            "foo foo foo foo foo foo foo foo foo"));
            */
            hook++;
            while (hook < yyLine->length()) {
                if (!yyLine->at(hook).isSpace())
                    return columnForIndex(*yyLine, hook);
                hook++;
            }
            return indentOfLine(*yyLine) + ppContinuationIndentSize;
        }

        if (braceDepth != 0)
            break;

        /*
            The line's delimiters are balanced. It looks like a
            continuation line or something.
        */
        if (delimDepth == 0) {
            if (leftBraceFollowed) {
                /*
                    We have

                        int main()
                        {

                    or

                        Bar::Bar()
                            : Foo(x)
                        {

                    The "{" should be flush left.
                */
                if (!isContinuationLine())
                    return indentOfLine(*yyLine);
            } else if (isContinuationLine() || yyLine->endsWith(QLatin1String(","))) {
                /*
                    We have

                        x = a +
                            b +
                            c;

                    or

                        int t[] = {
                            1, 2, 3,
                            4, 5, 6

                    The "c;" should fall right under the "b +", and the
                    "4, 5, 6" right under the "1, 2, 3,".
                */
                return indentOfLine(*yyLine);
            } else {
                /*
                    We have

                        stream << 1 +
                                2;

                    We could, but we don't, try to analyze which
                    operator has precedence over which and so on, to
                    obtain the excellent result

                        stream << 1 +
                                  2;

                    We do have a special trick above for the assignment
                    operator above, though.
                */
                return indentOfLine(*yyLine) + ppContinuationIndentSize;
            }
        }

        if (!readLine())
            break;
    }
    return 0;
}

/*
    Returns the recommended indent for the bottom line of yyProgram if
    that line is standalone (or should be indented likewise).

    Indenting a standalone line is tricky, mostly because of braceless
    control statements. Grossly, we are looking backwards for a special
    line, a "hook line", that we can use as a starting point to indent,
    and then modify the indentation level according to the braces met
    along the way to that hook.

    Let's consider a few examples. In all cases, we want to indent the
    bottom line.

    Example 1:

        x = 1;
        y = 2;

    The hook line is "x = 1;". We met 0 opening braces and 0 closing
    braces. Therefore, "y = 2;" inherits the indent of "x = 1;".

    Example 2:

        if (x) {
            y;

    The hook line is "if (x) {". No matter what precedes it, "y;" has
    to be indented one level deeper than the hook line, since we met one
    opening brace along the way.

    Example 3:

        if (a)
            while (b) {
                c;
            }
        d;

    To indent "d;" correctly, we have to go as far as the "if (a)".
    Compare with

        if (a) {
            while (b) {
                c;
            }
            d;

    Still, we're striving to go back as little as possible to
    accommodate people with irregular indentation schemes. A hook line
    near at hand is much more reliable than a remote one.
*/
int QScriptIndenter::indentForStandaloneLine()
{
    for (int i = 0; i < SmallRoof; i++) {
        if (!*yyLeftBraceFollows) {
            YY_SAVE();

            if (matchBracelessControlStatement()) {
                /*
                    The situation is this, and we want to indent "z;":

                        if (x &&
                             y)
                            z;

                    yyLine is "if (x &&".
                */
                return indentOfLine(*yyLine) + ppIndentSize;
            }
            YY_RESTORE();
        }

        if (yyLine->endsWith(QLatin1Char(';')) || yyLine->contains(QLatin1Char('{'))) {
            /*
                The situation is possibly this, and we want to indent
                "z;":

                    while (x)
                        y;
                    z;

                We return the indent of "while (x)". In place of "y;",
                any arbitrarily complex compound statement can appear.
            */

            if (*yyBraceDepth > 0) {
                do {
                    if (!readLine())
                        break;
                } while (*yyBraceDepth > 0);
            }

            LinizerState hookState;

            while (isContinuationLine())
                readLine();
            hookState = yyLinizerState;

            readLine();
            if (*yyBraceDepth <= 0) {
                do {
                    if (!matchBracelessControlStatement())
                        break;
                    hookState = yyLinizerState;
                } while (readLine());
            }

            yyLinizerState = hookState;

            while (isContinuationLine())
                readLine();

            /*
              Never trust lines containing only '{' or '}', as some
              people (Richard M. Stallman) format them weirdly.
            */
            if (yyLine->trimmed().length() > 1)
                return indentOfLine(*yyLine) - *yyBraceDepth * ppIndentSize;
        }

        if (!readLine())
            return -*yyBraceDepth * ppIndentSize;
    }
    return 0;
}

/*
    Returns the recommended indent for the bottom line of program.
    Unless null, typedIn stores the character of yyProgram that
    triggered reindentation.

    This function works better if typedIn is set properly; it is
    slightly more conservative if typedIn is completely wild, and
    slighly more liberal if typedIn is always null. The user might be
    annoyed by the liberal behavior.
*/
int QScriptIndenter::indentForBottomLine(QTextBlock begin, QTextBlock end, QChar typedIn)
{
    if (begin == end)
        return 0;

    yyProgram = Program(begin, end);
    startLinizer();

    const QTextBlock last = end.previous();

    QString bottomLine = last.text();
    QChar firstCh = firstNonWhiteSpace(bottomLine);
    int indent = 0;

    if (bottomLineStartsInMultilineComment()) {
        /*
            The bottom line starts in a C-style comment. Indent it
            smartly, unless the user has already played around with it,
            in which case it's better to leave her stuff alone.
        */
        if (isOnlyWhiteSpace(bottomLine)) {
            indent = indentWhenBottomLineStartsInMultiLineComment();
        } else {
            indent = indentOfLine(bottomLine);
        }
    } else if (okay(typedIn, QLatin1Char('#')) && firstCh == QLatin1Char('#')) {
        /*
            Preprocessor directives go flush left.
        */
        indent = 0;
    } else {
        if (isUnfinishedLine()) {
            indent = indentForContinuationLine();
        } else {
            indent = indentForStandaloneLine();
        }

        if (okay(typedIn, QLatin1Char('}')) && firstCh == QLatin1Char('}')) {
            /*
                A closing brace is one level more to the left than the
                code it follows.
            */
            indent -= ppIndentSize;
        } else if (okay(typedIn, QLatin1Char(':'))) {
            QRegExp caseLabel(
                QLatin1String("\\s*(?:case\\b(?:[^:]|::)+"
                              "|(?:default)\\s*"
                              ")?:.*"));

            if (caseLabel.exactMatch(bottomLine)) {
                /*
                    Move a case label (or the ':' in front of a
                    constructor initialization list) one level to the
                    left, but only if the user did not play around with
                    it yet. Some users have exotic tastes in the
                    matter, and most users probably are not patient
                    enough to wait for the final ':' to format their
                    code properly.

                    We don't attempt the same for goto labels, as the
                    user is probably the middle of "foo::bar". (Who
                    uses goto, anyway?)
                */
                if (indentOfLine(bottomLine) <= indent)
                    indent -= ppIndentSize;
                else
                    indent = indentOfLine(bottomLine);
            }
        }
    }

    return qMax(0, indent);
}
