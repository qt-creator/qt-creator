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

#include <qmljs/qmljsindenter.h>
#include <qmljs/qmljsscanner.h>

#include <QDebug>
#include <QTextBlock>

using namespace QmlJS;

/*
    Saves and restores the state of the global linizer. This enables
    backtracking.

    Identical to the defines in qmljslineinfo.cpp
*/
#define YY_SAVE() LinizerState savedState = yyLinizerState
#define YY_RESTORE() yyLinizerState = savedState


QmlJSIndenter::QmlJSIndenter()
    : caseOrDefault(QRegExp(QLatin1String(
            "\\s*(?:"
            "case\\b[^:]+|"
            "default)"
            "\\s*:.*")))

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
}

QmlJSIndenter::~QmlJSIndenter()
{
}

void QmlJSIndenter::setTabSize(int size)
{
    ppHardwareTabSize = size;
}

void QmlJSIndenter::setIndentSize(int size)
{
    ppIndentSize = size;
    ppContinuationIndentSize = 2 * size;
}

/*
    Returns true if string t is made only of white space; otherwise
    returns false.
*/
bool QmlJSIndenter::isOnlyWhiteSpace(const QString &t) const
{
    return firstNonWhiteSpace(t).isNull();
}

/*
    Assuming string t is a line, returns the column number of a given
    index. Column numbers and index are identical for strings that don't
    contain '\t's.
*/
int QmlJSIndenter::columnForIndex(const QString &t, int index) const
{
    int col = 0;
    if (index > t.length())
        index = t.length();

    for (int i = 0; i < index; i++) {
        if (t.at(i) == QLatin1Char('\t'))
            col = ((col / ppHardwareTabSize) + 1) * ppHardwareTabSize;
        else
            col++;
    }
    return col;
}

/*
    Returns the indentation size of string t.
*/
int QmlJSIndenter::indentOfLine(const QString &t) const
{
    return columnForIndex(t, t.indexOf(firstNonWhiteSpace(t)));
}

/*
    Replaces t[k] by ch, unless t[k] is '\t'. Tab characters are better
    left alone since they break the "index equals column" rule. No
    provisions are taken against '\n' or '\r', which shouldn't occur in
    t anyway.
*/
void QmlJSIndenter::eraseChar(QString &t, int k, QChar ch) const
{
    if (t.at(k) != QLatin1Char('\t'))
        t[k] = ch;
}

/*
    Returns '(' if the last parenthesis is opening, ')' if it is
    closing, and QChar() if there are no parentheses in t.
*/
QChar QmlJSIndenter::lastParen() const
{
    for (int index = yyLinizerState.tokens.size() - 1; index != -1; --index) {
        const Token &token = yyLinizerState.tokens.at(index);

        if (token.is(Token::LeftParenthesis))
            return QLatin1Char('(');

        else if (token.is(Token::RightParenthesis))
            return QLatin1Char(')');
    }

    return QChar();
}

/*
    Returns true if typedIn the same as okayCh or is null; otherwise
    returns false.
*/
bool QmlJSIndenter::okay(QChar typedIn, QChar okayCh) const
{
    return typedIn == QChar() || typedIn == okayCh;
}

/*
    Returns the recommended indent for the bottom line of yyProgram
    assuming that it starts in a C-style comment, a condition that is
    tested elsewhere.

    Essentially, we're trying to align against some text on the
    previous line.
*/
int QmlJSIndenter::indentWhenBottomLineStartsInMultiLineComment()
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
    Returns the recommended indent for the bottom line of yyProgram,
    assuming it's a continuation line.

    We're trying to align the continuation line against some parenthesis
    or other bracked left opened on a previous line, or some interesting
    operator such as '='.
*/
int QmlJSIndenter::indentForContinuationLine()
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
                    if (j < yyLine->length() - 1)
                        hook = j;
                    else
                        return 0; // shouldn't happen
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
                     j + 1 < yyLine->length() && yyLine->at(j + 1) != QLatin1Char('=')) {
                    if (braceDepth == 0 && delimDepth == 0 &&
                         j < yyLine->length() - 1 &&
                         !yyLine->endsWith(QLatin1Char(',')) &&
                         (yyLine->contains(QLatin1Char('(')) == yyLine->contains(QLatin1Char(')'))))
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
int QmlJSIndenter::indentForStandaloneLine()
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

            int indentChange = - *yyBraceDepth;
            if (caseOrDefault.exactMatch(*yyLine))
                ++indentChange;

            /*
              Never trust lines containing only '{' or '}', as some
              people (Richard M. Stallman) format them weirdly.
            */
            if (yyLine->trimmed().length() > 1)
                return indentOfLine(*yyLine) + indentChange * ppIndentSize;
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
int QmlJSIndenter::indentForBottomLine(QTextBlock begin, QTextBlock end, QChar typedIn)
{
    if (begin == end)
        return 0;

    const QTextBlock last = end.previous();

    initialize(begin, last);

    QString bottomLine = last.text();
    QChar firstCh = firstNonWhiteSpace(bottomLine);
    int indent = 0;

    if (bottomLineStartsInMultilineComment()) {
        /*
            The bottom line starts in a C-style comment. Indent it
            smartly, unless the user has already played around with it,
            in which case it's better to leave her stuff alone.
        */
        if (isOnlyWhiteSpace(bottomLine))
            indent = indentWhenBottomLineStartsInMultiLineComment();
        else
            indent = indentOfLine(bottomLine);
    } else {
        if (isUnfinishedLine())
            indent = indentForContinuationLine();
        else
            indent = indentForStandaloneLine();

        if ((okay(typedIn, QLatin1Char('}')) && firstCh == QLatin1Char('}'))
            || (okay(typedIn, QLatin1Char(']')) && firstCh == QLatin1Char(']'))) {
            /*
                A closing brace is one level more to the left than the
                code it follows.
            */
            indent -= ppIndentSize;
        } else if (okay(typedIn, QLatin1Char(':'))) {
            if (caseOrDefault.exactMatch(bottomLine)) {
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

