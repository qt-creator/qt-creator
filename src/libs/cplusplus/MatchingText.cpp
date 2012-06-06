/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/
#include "MatchingText.h"
#include "BackwardsScanner.h"

#include <Token.h>

#include <QTextDocument>
#include <QTextCursor>

#include <QChar>
#include <QtDebug>

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

// disable gcc warning:
//
//   qstring.h:1175:39: warning: assuming signed overflow does not occur when assuming that (X - c) > X is always false
//
// caused by Q_ASSERT in QStringRef::at()
#if defined(Q_CC_GNU) && !defined(Q_CC_INTEL)
#    pragma GCC diagnostic ignored "-Wstrict-overflow"
#endif

static bool isCompleteStringLiteral(const BackwardsScanner &tk, int index)
{
    const QStringRef text = tk.textRef(index);

    if (text.length() < 2)
        return false;

    else if (text.at(text.length() - 1) == QLatin1Char('"'))
        return text.at(text.length() - 2) != QLatin1Char('\\'); // ### not exactly.

    return false;
}

static bool isCompleteCharLiteral(const BackwardsScanner &tk, int index)
{
    const QStringRef text = tk.textRef(index);

    if (text.length() < 2)
        return false;

    else if (text.at(text.length() - 1) == QLatin1Char('\''))
        return text.at(text.length() - 2) != QLatin1Char('\\'); // ### not exactly.

    return false;
}

bool MatchingText::shouldInsertMatchingText(const QTextCursor &tc)
{
    QTextDocument *doc = tc.document();
    return shouldInsertMatchingText(doc->characterAt(tc.selectionEnd()));
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

QString MatchingText::insertMatchingBrace(const QTextCursor &cursor, const QString &textToProcess,
                                          QChar la, int *skippedChars) const
{
    QTextCursor tc = cursor;
    QTextDocument *doc = tc.document();
    QString text = textToProcess;

    const QString blockText = tc.block().text().mid(tc.positionInBlock());
    const QString trimmedBlockText = blockText.trimmed();
    const int length = qMin(blockText.length(), textToProcess.length());

    const QChar previousChar = doc->characterAt(tc.selectionEnd() - 1);

    bool escape = false;

    if (! text.isEmpty() && (text.at(0) == QLatin1Char('"') ||
                             text.at(0) == QLatin1Char('\''))) {
        if (previousChar == QLatin1Char('\\')) {
            int escapeCount = 0;
            int index = tc.selectionEnd() - 1;
            do {
                ++escapeCount;
                --index;
            } while (doc->characterAt(index) == QLatin1Char('\\'));

            if ((escapeCount % 2) != 0)
                escape = true;
        }
    }

    if (! escape) {
        for (int i = 0; i < length; ++i) {
            const QChar ch1 = blockText.at(i);
            const QChar ch2 = textToProcess.at(i);

            if (ch1 != ch2)
                break;
            else if (! shouldOverrideChar(ch1))
                break;

            ++*skippedChars;
        }
    }

    if (*skippedChars != 0) {
        tc.movePosition(QTextCursor::NextCharacter, QTextCursor::MoveAnchor, *skippedChars);
        text = textToProcess.mid(*skippedChars);
    }

    if (text.isEmpty() || !shouldInsertMatchingText(la))
        return QString();

    BackwardsScanner tk(tc, MAX_NUM_LINES, textToProcess.left(*skippedChars));
    const int startToken = tk.startToken();
    int index = startToken;

    const Token &token = tk[index - 1];

    if (text.at(0) == QLatin1Char('"') && token.isStringLiteral()) {
        if (text.length() != 1)
            qWarning() << Q_FUNC_INFO << "handle event compression";

        if (isCompleteStringLiteral(tk, index - 1))
            return QLatin1String("\"");

        return QString();
    } else if (text.at(0) == QLatin1Char('\'') && token.isCharLiteral()) {
        if (text.length() != 1)
            qWarning() << Q_FUNC_INFO << "handle event compression";

        if (isCompleteCharLiteral(tk, index - 1))
            return QLatin1String("'");

        return QString();
    }

    QString result;

    foreach (const QChar &ch, text) {
        if      (ch == QLatin1Char('('))  result += QLatin1Char(')');
        else if (ch == QLatin1Char('['))  result += QLatin1Char(']');
        else if (ch == QLatin1Char('"'))  result += QLatin1Char('"');
        else if (ch == QLatin1Char('\'')) result += QLatin1Char('\'');
        // Handle '{' appearance within functinon call context
        else if (ch == QLatin1Char('{') && !trimmedBlockText.isEmpty() && trimmedBlockText.at(0) == QLatin1Char(')'))
            result += QLatin1Char('}');

    }

    return result;
}

bool MatchingText::shouldInsertNewline(const QTextCursor &tc) const
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

QString MatchingText::insertParagraphSeparator(const QTextCursor &tc) const
{
    BackwardsScanner tk(tc, MAX_NUM_LINES);
    int index = tk.startToken();

    if (tk[index - 1].isNot(T_LBRACE))
        return QString(); // nothing to do.

    const QString textBlock = tc.block().text().mid(tc.positionInBlock()).trimmed();
    if (! textBlock.isEmpty())
        return QString();

    --index; // consume the `{'

    const Token &token = tk[index - 1];

    if (token.is(T_STRING_LITERAL) && tk[index - 2].is(T_EXTERN)) {
        // recognized extern "C"
        return QLatin1String("}");

    } else if (token.is(T_IDENTIFIER)) {
        int i = index - 1;

        forever {
            const Token &current = tk[i - 1];

            if (current.is(T_EOF_SYMBOL))
                break;

            else if (current.is(T_CLASS) || current.is(T_STRUCT) || current.is(T_UNION) || current.is(T_ENUM)) {
                // found a class key.
                QString str = QLatin1String("};");

                if (shouldInsertNewline(tc))
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
    }

    if (token.is(T_NAMESPACE)) {
        // anonymous namespace
        return QLatin1String("}");

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
            // found an unmatched brace. We don't really know to do in this case.
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

        if (shouldInsertNewline(tc))
            str += QLatin1Char('\n');

        return str;
    }

    // match the block
    return QLatin1String("}");
}
